#include "grpc_server/ClientServiceImpl.h"
#include "util/Time.h"

#include "google/protobuf/any.pb.h"

#include <spdlog/spdlog.h>

#include <thread>
#include <chrono>

namespace zion::grpc {

// Queue capacity: 1024 work items.  Each item holds a single TestResult.
static constexpr std::size_t kQueueCapacity = 1024;

ClientServiceImpl::ClientServiceImpl(RunService& run_svc, LogService& log_svc)
    : run_svc_(run_svc), log_svc_(log_svc) {}

// ── Handle a TestResultBatch ──────────────────────────────────────────────────

void ClientServiceImpl::handle_result_batch(
    const acadia::stream::TestResultBatch& batch,
    BoundedQueue<IngestWork>&              queue) {

    const std::string mode = std::to_string(static_cast<int>(batch.mode()));

    for (const auto& sample : batch.samples()) {
        const std::string type_url = sample.payload().type_url();
        if (type_url.empty()) {
            spdlog::debug("client_svc: empty payload type_url run_id={} module_id={}",
                          batch.run_id(), batch.module_id());
            continue;
        }

        constexpr std::string_view expected = "type.googleapis.com/hal.common.TestResult";
        if (type_url != expected) {
            spdlog::warn("client_svc: unexpected payload type_url='{}' run_id={}",
                         type_url, batch.run_id());
            continue;
        }

        hal::common::TestResult tr;
        if (!sample.payload().UnpackTo(&tr)) {
            spdlog::error("client_svc: failed to unpack TestResult run_id={} module_id={}",
                          batch.run_id(), batch.module_id());
            continue;
        }

        // UNSPECIFIED guard.
        if (tr.output().status() == hal::common::TEST_STATUS_UNSPECIFIED) {
            spdlog::warn("client_svc: UNSPECIFIED status rejected run_id={} module_id={}",
                         batch.run_id(), batch.module_id());
            continue;
        }

        IngestWork work;
        work.run_id         = batch.run_id();
        work.module_id      = batch.module_id();
        work.module_version = "";  // Not provided in batch; resolved from registry at ingest.
        work.mode           = mode;
        work.tr             = std::move(tr);

        if (!queue.enqueue(std::move(work))) {
            // Queue closed — stream is shutting down.
            spdlog::info("client_svc: queue closed, dropping sample run_id={}", batch.run_id());
            return;
        }
    }
}

// ── Handle a LogBatch ────────────────────────────────────────────────────────

void ClientServiceImpl::handle_log_batch(const acadia::stream::LogBatch& batch) {
    for (const auto& line : batch.lines()) {
        LogEntry e;
        e.run_id   = line.run_id().empty() ? batch.run_id() : line.run_id();
        e.source   = line.source();
        e.severity = line.level();
        e.message  = line.text();
        log_svc_.ingest(e);
    }
}

// ── Connect (bidi stream) ─────────────────────────────────────────────────────

::grpc::Status ClientServiceImpl::Connect(
    ::grpc::ServerContext* ctx,
    ::grpc::ServerReaderWriter<acadia::stream::ServerMessage,
                                acadia::stream::ClientMessage>* stream) {

    BoundedQueue<IngestWork> queue{kQueueCapacity};
    std::atomic<bool> writer_ok{true};

    // DB writer thread: drains the queue and persists results.
    std::thread writer_thread{[&] {
        while (true) {
            auto work = queue.dequeue();
            if (!work.has_value()) break;  // Queue closed.
            try {
                run_svc_.ingest_test_result(work->tr, work->run_id,
                                             work->module_version, work->mode);
            } catch (const std::exception& e) {
                spdlog::error("client_svc: ingest error run_id={}: {}",
                              work->run_id, e.what());
                // Do not abort the stream for a single failed ingest.
            }
        }
    }};

    acadia::stream::ClientMessage msg;
    while (!ctx->IsCancelled() && stream->Read(&msg)) {
        switch (msg.payload_case()) {
        case acadia::stream::ClientMessage::kHello: {
            const auto& hello = msg.hello();
            spdlog::info("client_svc: HelloReport board_id={} client_ver={}",
                         hello.board_id(), hello.client_version());
            break;
        }

        case acadia::stream::ClientMessage::kResultBatch: {
            const auto& batch = msg.result_batch();
            handle_result_batch(batch, queue);

            // Send BatchAck.
            acadia::stream::ServerMessage ack_msg;
            auto* ack = ack_msg.mutable_batch_ack();
            ack->set_run_id(batch.run_id());
            ack->set_batch_seq(batch.batch_seq());
            if (!stream->Write(ack_msg)) {
                spdlog::warn("client_svc: failed to send BatchAck run_id={}", batch.run_id());
            }

            // If terminal batch, finalize the run.
            if (batch.terminal()) {
                spdlog::info("client_svc: terminal batch run_id={} module_id={}",
                             batch.run_id(), batch.module_id());
                run_svc_.complete_run(batch.run_id(), "completed");
            }
            break;
        }

        case acadia::stream::ClientMessage::kLogBatch: {
            const auto& batch = msg.log_batch();
            handle_log_batch(batch);

            // Send LogBatchAck.
            acadia::stream::ServerMessage ack_msg;
            auto* ack = ack_msg.mutable_log_batch_ack();
            ack->set_run_id(batch.run_id());
            ack->set_batch_seq(batch.batch_seq());
            if (!stream->Write(ack_msg)) {
                spdlog::warn("client_svc: failed to send LogBatchAck run_id={}", batch.run_id());
            }
            break;
        }

        case acadia::stream::ClientMessage::kStateReport: {
            // Log heartbeat; no response required.
            spdlog::debug("client_svc: StateReport board_id={}",
                          msg.state_report().board_id());
            break;
        }

        case acadia::stream::ClientMessage::kConfigureAck: {
            const auto& ack = msg.configure_ack();
            if (!ack.ok()) {
                spdlog::warn("client_svc: ConfigureAck NACK run_id={} field={} error={}",
                             ack.run_id(), ack.field(), ack.error());
            }
            break;
        }

        default:
            spdlog::debug("client_svc: unknown payload_case={}", msg.payload_case());
            break;
        }
    }

    // Close the queue and wait for writer thread to flush.
    queue.close();
    if (writer_thread.joinable()) writer_thread.join();

    spdlog::info("client_svc: stream ended (cancelled={})", ctx->IsCancelled());
    return ::grpc::Status::OK;
}

}  // namespace zion::grpc
