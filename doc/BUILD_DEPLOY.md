# Zion-Server Build and Deployment

## Purpose and scope
Zion-Server is the C++20 local orchestration/data-plane service. It generates protobuf/gRPC sources from sibling Zion-HAL protos, builds REST/gRPC services and tests, applies PostgreSQL migrations at startup, and exposes HTTP, gRPC, and optional metrics endpoints.

## 1. Prerequisites
- Linux with CMake >= 3.22, C++20 compiler, Make, protobuf/protoc, `grpc_cpp_plugin`, Drogon, spdlog, yaml-cpp, GoogleTest, OpenSSL, Argon2, and PostgreSQL client/runtime dependencies.
- The sibling `../Zion-HAL/protos` tree is required for code generation.
- Docker/Compose is recommended for PostgreSQL and the development toolchain; native setup may use `local_install.sh` and `VCPKG_ROOT`.

```bash
cp .env.example .env
```
Set a real database password and a session secret of at least 32 characters; do not commit `.env`.

## 2. Setup
Copy `.env.example` to `.env` and configure local database and session values.

## 3. Build and compile
```bash
cmake --preset debug
cmake --build build/debug
cmake --preset release
cmake --build build/release
cmake --preset asan
cmake --build build/asan
```
The `test` preset enables coverage instrumentation. Docker setup is `cd docker && ./setup.sh`, followed by building inside `zion-dev`; the Docker Makefile also provides `make check`, `make build`, `make build-amd64`, `make build-arm64`, and `make up`.

## 4. Test and lint
```bash
cmake --preset test
cmake --build build/test
ctest --test-dir build/test --output-on-failure
cmake --build build/test --target config-test
```
CMake discovers unit and integration GoogleTest binaries when their source trees contain tests. The Makefile has no lint target; use the repository's configured compiler warnings and any CI checks. `asan` is a conditional sanitizer build and may require compatible runtime/toolchain support.

## 5. Packaging and artifacts
The installable executable is `build/<preset>/zion-server`; CMake installs it to `bin`. Generated protobuf and version files are under the selected build's `generated/` directory. PostgreSQL schema resources remain in `migrations/`; the repository has no top-level release packaging target.

## 6. Deployment, install, and run
```bash
docker compose -f docker/docker-compose.yml up postgres -d
./scripts/run_dev.sh
./build/debug/zion-server --config etc/zion/server.toml --log-level debug
```
For the full local container flow use `docker compose -f docker/docker-compose.yml up --build`. The `.env.example` defaults document HTTP 8080, gRPC 50051, optional metrics 9100, PostgreSQL 5432, registry path, OTA storage, CORS, and auth settings. `local_install.sh` is conditional, Ubuntu-focused, needs sudo/network/disk, and may build yaml-cpp, Drogon, and grpc_cpp_plugin from source.

## 7. Cleanup
Use the relevant CMake build directory removal or reconfigure it; Docker cleanup is through `cd docker && make down` and image cleanup targets. PostgreSQL data is in the Compose volume and is intentionally preserved by normal `down`; deleting volumes is destructive. Remove local `.env` and generated build outputs deliberately.

## 8. Hardware, cloud, and manual verification caveats
Compilation and unit tests do not prove PostgreSQL migrations, authenticated REST, gRPC streams from Zion-Client, Dashboard/SSE behavior, OTA storage/signatures, or production deployment. Credentials, database state, network ports, registry files, and device connectivity are conditional. Validate API/auth and destructive OTA operations only against an isolated development environment.
