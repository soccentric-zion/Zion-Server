cmake_minimum_required(VERSION 3.22)
project(zion-server VERSION 0.1.0 LANGUAGES CXX) set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) if(NOT CMAKE_BUILD_TYPE) set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif # ── Optional coverage ─────────────────────────────────────────────────────────
option(ENABLE_COVERAGE "Enable gcov coverage instrumentation" OFF)
if(ENABLE_COVERAGE) add_compile_options(--coverage -O0 -g) add_link_options(--coverage)
endif # ── Dependencies via find_package (system packages) ──────────────────────────
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED) # uses FindProtobuf.cmake → protobuf::libprotobuf
find_package(Drogon CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED) # yaml-cpp target name differs between package versions
if(TARGET yaml-cpp::yaml-cpp) set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
elseif(TARGET yaml-cpp) set(YAML_CPP_TARGET yaml-cpp)
else message(FATAL_ERROR "yaml-cpp imported target not found")
endif # prometheus-cpp (optional — expose metrics endpoint)
find_package(prometheus-cpp CONFIG QUIET) # libargon2
find_library(ARGON2_LIB argon2 REQUIRED)
find_path(ARGON2_INCLUDE argon2.h REQUIRED) # ── Proto/ gRPC code generation ──────────────────────────────────────────────
find_program(PROTOC_EXE NAMES protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN NAMES grpc_cpp_plugin REQUIRED) # Normalize to absolute path (protoc does not handle .. in paths)
get_filename_component(PROTO_SRC_DIR "${CMAKE_SOURCE_DIR}/../Zion-HAL/protos" ABSOLUTE
)
set(PROTO_GEN_DIR "${CMAKE_BINARY_DIR}/generated/proto")
file(MAKE_DIRECTORY "${PROTO_GEN_DIR}") # Only compile the two protos needed in the server.
# Module-specific protos are sent as google.protobuf.Any and don't need
# to be compiled into this binary.
set(PROTO_SRCS "")
set(PROTO_HDRS "") # --- common.proto (no gRPC stubs) ---
set(COMMON_PROTO "${PROTO_SRC_DIR}/common.proto")
set(COMMON_CC "${PROTO_GEN_DIR}/common.pb.cc")
set(COMMON_H "${PROTO_GEN_DIR}/common.pb.h")
add_custom_command(OUTPUT "${COMMON_CC}" "${COMMON_H}" COMMAND "${PROTOC_EXE}" --proto_path=${PROTO_SRC_DIR} --proto_path=/usr/include --cpp_out=${PROTO_GEN_DIR} ${COMMON_PROTO} DEPENDS "${COMMON_PROTO}" COMMENT "Generating protobuf C++ from common.proto" VERBATIM
)
list(APPEND PROTO_SRCS "${COMMON_CC}")
list(APPEND PROTO_HDRS "${COMMON_H}") # --- hal_service.proto (protobuf + gRPC stubs) ---
set(HAL_PROTO "${PROTO_SRC_DIR}/hal_service.proto")
set(HAL_PB_CC "${PROTO_GEN_DIR}/hal_service.pb.cc")
set(HAL_PB_H "${PROTO_GEN_DIR}/hal_service.pb.h")
set(HAL_GRPC_CC "${PROTO_GEN_DIR}/hal_service.grpc.pb.cc")
set(HAL_GRPC_H "${PROTO_GEN_DIR}/hal_service.grpc.pb.h")
add_custom_command(OUTPUT "${HAL_PB_CC}" "${HAL_PB_H}" "${HAL_GRPC_CC}" "${HAL_GRPC_H}" COMMAND "${PROTOC_EXE}" --proto_path=${PROTO_SRC_DIR} --proto_path=/usr/include --cpp_out=${PROTO_GEN_DIR} --grpc_out=${PROTO_GEN_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${HAL_PROTO} DEPENDS "${HAL_PROTO}" "${COMMON_PROTO}" COMMENT "Generating protobuf+gRPC C++ from hal_service.proto" VERBATIM
)
list(APPEND PROTO_SRCS "${HAL_PB_CC}" "${HAL_GRPC_CC}")
list(APPEND PROTO_HDRS "${HAL_PB_H}" "${HAL_GRPC_H}") # --- client_service.proto (protobuf + gRPC stubs) ---
# The on-target Client App connects to the Server via this bidi-streaming service.
# It sends TestResultBatch (with hal.common.TestResult payloads) and receives run commands.
set(CLIENT_PROTO "${PROTO_SRC_DIR}/client_service.proto")
set(CLIENT_PB_CC "${PROTO_GEN_DIR}/client_service.pb.cc")
set(CLIENT_PB_H "${PROTO_GEN_DIR}/client_service.pb.h")
set(CLIENT_GRPC_CC "${PROTO_GEN_DIR}/client_service.grpc.pb.cc")
set(CLIENT_GRPC_H "${PROTO_GEN_DIR}/client_service.grpc.pb.h")
add_custom_command(OUTPUT "${CLIENT_PB_CC}" "${CLIENT_PB_H}" "${CLIENT_GRPC_CC}" "${CLIENT_GRPC_H}" COMMAND "${PROTOC_EXE}" --proto_path=${PROTO_SRC_DIR} --proto_path=/usr/include --cpp_out=${PROTO_GEN_DIR} --grpc_out=${PROTO_GEN_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${CLIENT_PROTO} DEPENDS "${CLIENT_PROTO}" "${COMMON_PROTO}" COMMENT "Generating protobuf+gRPC C++ from client_service.proto" VERBATIM
)
list(APPEND PROTO_SRCS "${CLIENT_PB_CC}" "${CLIENT_GRPC_CC}")
list(APPEND PROTO_HDRS "${CLIENT_PB_H}" "${CLIENT_GRPC_H}") # Proto static library
add_library(zion-proto STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(zion-proto PUBLIC "${PROTO_GEN_DIR}")
target_link_libraries(zion-proto PUBLIC gRPC::grpc++ protobuf::libprotobuf
)
target_compile_options(zion-proto PRIVATE -Wno-unused-parameter -Wno-deprecated-declarations) # ── Version header ────────────────────────────────────────────────────────────
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")
configure_file("${CMAKE_SOURCE_DIR}/cmake/version.hpp.in" "${GENERATED_DIR}/version.hpp" @ONLY
) # ── Migration resources ───────────────────────────────────────────────────────
# Embed the migrations directory path so MigrationRunner can find SQL files.
set(MIGRATIONS_DIR "${CMAKE_SOURCE_DIR}/migrations") # ── Collect source files ───────────────────────────────────────────────────────
file(GLOB_RECURSE ZION_CORE_SRCS "${CMAKE_SOURCE_DIR}/src/config/*.cc" "${CMAKE_SOURCE_DIR}/src/logging/*.cc" "${CMAKE_SOURCE_DIR}/src/registry/*.cc" "${CMAKE_SOURCE_DIR}/src/db/*.cc" "${CMAKE_SOURCE_DIR}/src/repositories/*.cc" "${CMAKE_SOURCE_DIR}/src/services/*.cc" "${CMAKE_SOURCE_DIR}/src/grpc_server/*.cc" "${CMAKE_SOURCE_DIR}/src/rest_api/*.cc" "${CMAKE_SOURCE_DIR}/src/orchestration/*.cc" "${CMAKE_SOURCE_DIR}/src/util/*.cc"
) # ── Core library ──────────────────────────────────────────────────────────────
add_library(zion-core STATIC ${ZION_CORE_SRCS}) target_include_directories(zion-core PUBLIC "${CMAKE_SOURCE_DIR}/src" "${GENERATED_DIR}" "${ARGON2_INCLUDE}"
) target_compile_definitions(zion-core PUBLIC ZION_MIGRATIONS_DIR="${MIGRATIONS_DIR}"
) set(ZION_CORE_LIBS zion-proto Drogon::Drogon spdlog::spdlog ${YAML_CPP_TARGET} OpenSSL::SSL OpenSSL::Crypto "${ARGON2_LIB}" gRPC::grpc++ gRPC::grpc++_reflection protobuf::libprotobuf
) if(prometheus-cpp_FOUND) list(APPEND ZION_CORE_LIBS prometheus-cpp::pull prometheus-cpp::core) target_compile_definitions(zion-core PUBLIC ZION_HAS_PROMETHEUS=1)
endif target_link_libraries(zion-core PUBLIC ${ZION_CORE_LIBS}) target_compile_options(zion-core PRIVATE -Wall -Wextra -Wno-unused-parameter) # ── Main binary ───────────────────────────────────────────────────────────────
add_executable(zion-server src/main.cpp)
target_link_libraries(zion-server PRIVATE zion-core)
target_compile_options(zion-server PRIVATE -Wall -Wextra -Wno-unused-parameter)
install(TARGETS zion-server DESTINATION bin) # ── Unit tests ────────────────────────────────────────────────────────────────
enable_testing file(GLOB_RECURSE UNIT_TEST_SRCS "${CMAKE_SOURCE_DIR}/tests/unit/*.cc")
if(UNIT_TEST_SRCS) add_executable(zion-unit-tests ${UNIT_TEST_SRCS}) target_include_directories(zion-unit-tests PRIVATE "${CMAKE_SOURCE_DIR}/src" "${GENERATED_DIR}" "${ARGON2_INCLUDE}") target_link_libraries(zion-unit-tests PRIVATE zion-core GTest::gtest GTest::gtest_main) target_compile_options(zion-unit-tests PRIVATE -Wall -Wextra -Wno-unused-parameter) include(GoogleTest) gtest_discover_tests(zion-unit-tests)
endif # ── Integration tests ─────────────────────────────────────────────────────────
file(GLOB_RECURSE INTEG_TEST_SRCS "${CMAKE_SOURCE_DIR}/tests/integration/*.cc")
if(INTEG_TEST_SRCS) add_executable(zion-integration-tests ${INTEG_TEST_SRCS}) target_include_directories(zion-integration-tests PRIVATE "${CMAKE_SOURCE_DIR}/src" "${GENERATED_DIR}" "${ARGON2_INCLUDE}") target_link_libraries(zion-integration-tests PRIVATE zion-core GTest::gtest GTest::gtest_main) target_compile_options(zion-integration-tests PRIVATE -Wall -Wextra -Wno-unused-parameter) include(GoogleTest) gtest_discover_tests(zion-integration-tests PROPERTIES LABELS "integration")
endif # ── Legacy internal lib + config-test (kept for compatibility) ─────────────────
set(FETCHCONTENT_UPDATES_DISCONNECTED ON) # reuse cached download
include(FetchContent)
FetchContent_Declare(tomlplusplus GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git GIT_TAG v3.4.0 GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(tomlplusplus) add_library(zion-internal STATIC internal/config/config.cpp internal/logging/logger.cpp
)
target_include_directories(zion-internal PUBLIC "${CMAKE_SOURCE_DIR}" "${GENERATED_DIR}"
)
target_link_libraries(zion-internal PUBLIC tomlplusplus::tomlplusplus)
target_compile_options(zion-internal PRIVATE -Wall -Wextra -Wno-unused-parameter) add_executable(config-test test/config_test.cpp)
target_link_libraries(config-test PRIVATE zion-internal)
target_compile_options(config-test PRIVATE -Wall -Wextra)
add_test(NAME config-test COMMAND config-test)
