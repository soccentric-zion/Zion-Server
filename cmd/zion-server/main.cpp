#include "internal/config/config.hpp"
#include "internal/logging/logger.hpp"
#include "version.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

static std::atomic<bool> g_shutdown{false};

static void on_signal(int /*sig*/) {
    g_shutdown.store(true, std::memory_order_relaxed);
}

static void install_signal_handlers() {
    struct sigaction sa{};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    // SIGPIPE: will arrive from broken gRPC/WebSocket connections — ignore it.
    signal(SIGPIPE, SIG_IGN);
}

static void print_usage(const char* argv0) {
    std::cout
        << "Usage: " << argv0
        << " [--config PATH] [--log-level LEVEL] [--version] [--help]\n"
        << "\n"
        << "  --config, -c PATH      TOML config file (default: /etc/zion/server.toml)\n"
        << "  --log-level, -l LEVEL  debug|info|warn|error|critical (default: info)\n"
        << "  --version, -v          Print version and exit\n"
        << "  --help, -h             Print this help and exit\n";
}

int main(int argc, char* argv[]) {
    std::string config_path = "/etc/zion/server.toml";
    std::string log_level   = "info";

    for (int i = 1; i < argc; ++i) {
        std::string_view arg{argv[i]};
        if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            config_path = argv[++i];
        } else if ((arg == "--log-level" || arg == "-l") && i + 1 < argc) {
            log_level = argv[++i];
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "zion-server " << ZION_SERVER_VERSION << "\n";
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    zion::log::init(log_level);

    zion::config::Config cfg;
    try {
        cfg = zion::config::load(config_path);
        zion::log::info("config loaded", {{"path", config_path}});
    } catch (const std::exception& e) {
        zion::log::warn("config load failed — using defaults",
                          {{"path", config_path}, {"reason", e.what()}});
        cfg = zion::config::defaults();
    }

    zion::log::info("zion-server starting",
        {{"version",      ZION_SERVER_VERSION},
         {"grpc_listen",  cfg.server.grpc_listen},
         {"http_listen",  cfg.server.http_listen},
         {"db_path",      cfg.storage.db_path},
         {"metrics",      cfg.metrics.enabled ? cfg.metrics.listen : "disabled"}});

    install_signal_handlers();

    // Placeholder: future steps will start gRPC, HTTP, and ingest subsystems here.
    zion::log::info("server ready — awaiting shutdown signal");

    while (!g_shutdown.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    zion::log::info("shutdown signal received");
    // Future: signal gRPC server to stop, drain ingest queue, flush DB writers.
    zion::log::info("server stopped cleanly");

    return 0;
}
