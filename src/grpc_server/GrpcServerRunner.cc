#include "grpc_server/GrpcServerRunner.h"

#include <spdlog/spdlog.h>
#include <grpcpp/server_builder.h>
#include <stdexcept>

namespace zion::grpc {

GrpcServerRunner::GrpcServerRunner(HalServiceImpl&    hal_service,
                                    ClientServiceImpl& client_service,
                                    const NetworkSettings& net)
    : hal_service_(hal_service),
      client_service_(client_service),
      net_(net) {}

void GrpcServerRunner::start() {
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(net_.grpc_listen,
                             ::grpc::InsecureServerCredentials());
    builder.RegisterService(&hal_service_);
    builder.RegisterService(&client_service_);

    // Reuse port, keepalive tuning.
    builder.SetMaxReceiveMessageSize(32 * 1024 * 1024);  // 32 MiB
    builder.SetMaxSendMessageSize(32 * 1024 * 1024);

    server_ = builder.BuildAndStart();
    if (!server_) {
        throw std::runtime_error("Failed to start gRPC server on " + net_.grpc_listen);
    }
    spdlog::info("gRPC server listening on {} (HalService + ClientService)", net_.grpc_listen);
    server_->Wait();
}

void GrpcServerRunner::shutdown() {
    if (server_) {
        spdlog::info("gRPC server shutting down");
        server_->Shutdown();
    }
}

}  // namespace zion::grpc
