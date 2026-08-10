// src/main.cpp — Zion Server entry point.
// Starts gRPC + Drogon (HTTP/REST) servers after running DB migrations.

#include "config/Settings.h"
#include "logging/LoggingSetup.h"
#include "db/DbClient.h"
#include "db/MigrationRunner.h"
#include "registry/ModuleRegistry.h"

#include "services/AuthService.h"
#include "services/RunService.h"
#include "services/LogService.h"
#include "services/ModuleService.h"
#include "services/TargetService.h"
#include "services/ConfigService.h"
#include "services/TemplateService.h"
#include "services/ScheduleService.h"
#include "services/OtaService.h"
#include "services/NotificationService.h"
#include "services/ThresholdService.h"
#include "services/AnomalyService.h"
#include "services/CompareService.h"

#include "grpc_server/HalServiceImpl.h"
#include "grpc_server/ClientServiceImpl.h"
#include "grpc_server/GrpcServerRunner.h"

#include "rest_api/DrogonAppFactory.h"

#include "orchestration/RunManager.h"
#include "orchestration/Scheduler.h"
#include "orchestration/TargetHeartbeat.h"
#include "orchestration/OtaOrchestrator.h"
#include "orchestration/RetentionScheduler.h"

#include <spdlog/spdlog.h>
#include <drogon/drogon.h>

#include <csignal>
#include <atomic>
#include <thread>
#include <filesystem>

// ------------------------------------------------------------------
// Signal handling
// ------------------------------------------------------------------
namespace {
std::atomic<bool> g_shutdown{false};

void handle_signal(int /*sig*/) {
    g_shutdown = true;
    drogon::app().quit();
}
}  // namespace

int main() {
    // --- Load settings from environment ---
    zion::Settings settings;
    try {
        settings = zion::load_settings();
    } catch (const std::exception& e) {
        spdlog::critical("Failed to load settings: {}", e.what());
        return 1;
    }

    // --- Logging ---
    zion::logging::setup_logging(settings.log_level);
    spdlog::info("Zion Server starting — {}", zion::settings_summary(settings));

    // --- Database ---
    try {
        zion::db::init_db_client(settings.db);
    } catch (const std::exception& e) {
        spdlog::critical("DB init failed: {}", e.what());
        return 1;
    }

    // Run migrations.
    zion::db::MigrationRunner migrations{ZION_MIGRATIONS_DIR};
    try {
        migrations.run();
    } catch (const std::exception& e) {
        spdlog::critical("Migration failed: {}", e.what());
        return 1;
    }

    // --- Module Registry ---
    zion::ModuleRegistry registry;
    if (!settings.registry_path.empty() &&
        std::filesystem::exists(settings.registry_path)) {
        registry.load(settings.registry_path);
        spdlog::info("Module registry loaded: {} modules", registry.size());
    } else {
        spdlog::warn("No module registry found at '{}'", settings.registry_path);
    }

    // --- Services ---
    zion::AuthService          auth_svc{settings.auth};
    zion::RunService           run_svc;
    zion::LogService           log_svc;
    zion::ModuleService        mod_svc{registry};
    zion::TargetService        target_svc;
    zion::ConfigService        config_svc;
    zion::TemplateService      template_svc;
    zion::ScheduleService      schedule_svc;
    zion::OtaService           ota_svc{settings.ota};
    zion::NotificationService  notif_svc{settings.smtp};
    zion::ThresholdService     threshold_svc;
    zion::AnomalyService       anomaly_svc;
    zion::CompareService       compare_svc;

    // --- Orchestration ---
    zion::orchestration::RunManager      run_mgr{run_svc, log_svc, notif_svc};
    zion::orchestration::Scheduler       scheduler{schedule_svc, run_svc};
    zion::orchestration::TargetHeartbeat heartbeat{target_svc};
    zion::orchestration::OtaOrchestrator ota_orch{ota_svc};
    zion::RetentionScheduler             retention{run_svc};

    // Reconcile any runs that were left in 'running'/'queued' state from a previous crash.
    {
        int stale = run_svc.reconcile_stale_runs();
        if (stale > 0) {
            spdlog::warn("Reconciled {} stale run(s) to 'failed' on startup", stale);
        }
    }

    scheduler.start();
    heartbeat.start();
    ota_orch.start();
    retention.start();

    // --- gRPC Server (in background thread) ---
    zion::grpc::HalServiceImpl    hal_service{run_svc, log_svc, mod_svc, settings};
    zion::grpc::ClientServiceImpl client_service{run_svc, log_svc};
    zion::grpc::GrpcServerRunner  grpc_runner{hal_service, client_service, settings.network};

    std::thread grpc_thread([&] {
        try {
            grpc_runner.start();
        } catch (const std::exception& e) {
            spdlog::critical("gRPC server error: {}", e.what());
            g_shutdown = true;
            drogon::app().quit();
        }
    });

    // --- Signals ---
    std::signal(SIGINT,  handle_signal);
    std::signal(SIGTERM, handle_signal);

    // --- REST API (Drogon — blocks until quit()) ---
    zion::rest::configure_drogon_app(settings,
                                        auth_svc,
                                        run_svc,
                                        log_svc,
                                        mod_svc,
                                        target_svc,
                                        config_svc,
                                        template_svc,
                                        schedule_svc,
                                        ota_svc,
                                        notif_svc,
                                        compare_svc);

    spdlog::info("Starting Drogon HTTP server on {}", settings.network.http_listen);
    drogon::app().run();  // blocks

    // --- Shutdown ---
    spdlog::info("Shutting down...");
    scheduler.stop();
    heartbeat.stop();
    retention.stop();
    ota_orch.stop();
    grpc_runner.shutdown();
    if (grpc_thread.joinable()) grpc_thread.join();

    spdlog::info("Zion Server stopped");
    return 0;
}
