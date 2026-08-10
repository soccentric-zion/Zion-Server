#pragma once
// grpc_server/GrpcServerRunner.h — owns and manages the gRPC server lifecycle.

#include "grpc_server/HalServiceImpl.h"
#include "grpc_server/ClientServiceImpl.h"
#include "config/Settings.h"

#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>

namespace zion::grpc {

class GrpcServerRunner {
public:
    GrpcServerRunner(HalServiceImpl&    hal_service,
                     ClientServiceImpl& client_service,
                     const NetworkSettings& net);

    // Start the gRPC server.  Blocks until shutdown() is called.
    void start();

    // Signal graceful shutdown.
    void shutdown();

private:
    HalServiceImpl&          hal_service_;
    ClientServiceImpl&       client_service_;
    NetworkSettings          net_;
    std::unique_ptr<::grpc::Server> server_;
};

}  // namespace zion::grpc
