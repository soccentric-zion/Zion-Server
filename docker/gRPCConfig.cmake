# gRPCConfig.cmake — wraps Ubuntu 22.04 system-installed gRPC 1.30.x libraries.
# Installed into the Docker image so that find_package(gRPC CONFIG REQUIRED) works
# without needing a full gRPC source build.

set(gRPC_VERSION "1.30.2")
set(gRPC_FOUND TRUE)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

if(NOT TARGET gRPC::gpr)
  add_library(gRPC::gpr SHARED IMPORTED)
  set_target_properties(gRPC::gpr PROPERTIES
    IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libgpr.so"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include")
endif()

if(NOT TARGET gRPC::grpc)
  add_library(gRPC::grpc SHARED IMPORTED)
  set_target_properties(gRPC::grpc PROPERTIES
    IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libgrpc.so"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include"
    INTERFACE_LINK_LIBRARIES "gRPC::gpr;OpenSSL::SSL;OpenSSL::Crypto;Threads::Threads")
endif()

if(NOT TARGET gRPC::grpc++)
  add_library(gRPC::grpc++ SHARED IMPORTED)
  set_target_properties(gRPC::grpc++ PROPERTIES
    IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libgrpc++.so"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include"
    INTERFACE_LINK_LIBRARIES "gRPC::grpc")
endif()

if(NOT TARGET gRPC::grpc++_reflection)
  add_library(gRPC::grpc++_reflection SHARED IMPORTED)
  set_target_properties(gRPC::grpc++_reflection PROPERTIES
    IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libgrpc++_reflection.so"
    INTERFACE_LINK_LIBRARIES "gRPC::grpc++")
endif()
