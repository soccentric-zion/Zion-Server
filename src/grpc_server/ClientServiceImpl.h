#pragma once
// grpc_server/ClientServiceImpl.h — implements the acadia::stream::ClientService bidi stream.
//
// The Client (on-target device) sends ClientMessage frames over a single persistent stream.
// This implementation:
//   - Processes TestResultBatch frames → extracts ResultSamples → unpacks each Any payload
//     as hal::common::TestResult and calls RunService::ingest_test_result.
//   - Processes LogBatch frames → forwards to LogService.
//   - Processes HelloReport → registers the board.
//   - Sends BatchAck / LogBatchAck responses.
//   - Applies BoundedQueue backpressure: the DB write happens on a dedicated writer thread;
//     the reader thread enqueues work items and blocks only when the queue is full.

#include "services/RunService.h"
#include "services/LogService.h"
#include "grpc_server/StreamBackpressure.h"

#include "client_service.grpc.pb.h"
#include "client_service.pb.h"
#include "common.pb.h"

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace zion::grpc {

class ClientServiceImpl final
    : public acadia::stream::ClientService::Service {
public:
    ClientServiceImpl(RunService& run_svc, LogService& log_svc);

    ::grpc::Status Connect(
        ::grpc::ServerContext*                                                    ctx,
        ::grpc::ServerReaderWriter<acadia::stream::ServerMessage,
                                   acadia::stream::ClientMessage>* stream) override;

private:
    struct IngestWork {
        std::string              run_id;
        std::string              module_id;
        std::string              module_version;
        std::string              mode;
        hal::common::TestResult  tr;
    };

    RunService& run_svc_;
    LogService& log_svc_;

    // Process a single TestResultBatch, enqueue each sample for DB write.
    void handle_result_batch(
        const acadia::stream::TestResultBatch& batch,
        BoundedQueue<IngestWork>&              queue);

    // Process a single LogBatch.
    void handle_log_batch(const acadia::stream::LogBatch& batch);
};

}  // namespace zion::grpc
