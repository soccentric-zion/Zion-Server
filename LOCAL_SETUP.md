# Zion Server — Local Development Setup & Dependencies

This document provides a comprehensive guide to understanding and installing all dependencies for the Zion Server project.

## 📋 Table of Contents

1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [System Requirements](#system-requirements)
4. [Dependencies Reference](#dependencies-reference)
5. [Installation Instructions](#installation-instructions)
6. [Post-Installation Setup](#post-installation-setup)
7. [Troubleshooting](#troubleshooting)
8. [Development Workflow](#development-workflow)

---

## Project Overview

**Zion Server** is the central orchestration and data-collection backend for the Zion IV&V (Independent Validation & Verification) tool suite. It provides:

- **gRPC Service** (`HalService`): Handles communication with embedded HAL Client App for test result submission and streaming command delivery
- **REST/HTTP API** (`/api/v1`): Exposes endpoints for Web UI and external tooling
- **PostgreSQL Backend**: Persistent data storage with versioned migrations
- **OTA Firmware Upgrade**: Orchestration and management of firmware updates
- **Prometheus Metrics** (Optional): Performance monitoring and metrics exposition
- **gRPC Reflection**: Dynamic service discovery and introspection

### Key Technologies

- **Language**: C++20 (GCC 12+ or Clang 16+)
- **Build System**: CMake 3.22+
- **Web Framework**: Drogon v1.9.3 (C++ REST framework with ORM)
- **RPC Framework**: gRPC v1.30.2 + Protocol Buffers
- **Database**: PostgreSQL 15+
- **Testing**: Google Test (GTest)
- **Logging**: spdlog (high-performance logging)
- **Configuration**: YAML and TOML formats

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Zion Server                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │               Network Layer                         │   │
│  ├─────────────────────────────────────────────────────┤   │
│  │  gRPC Listeners      │   HTTP/REST Listeners       │   │
│  │  (Port 9090)         │   (Port 8080)                │   │
│  └──────────┬───────────┴────────────┬──────────────────┘   │
│             │                        │                       │
│  ┌──────────┴────────────┬───────────┴──────────────────┐   │
│  │      gRPC Service     │      Drogon Web Layer       │   │
│  │  • HalServiceImpl      │  • REST API Controllers     │   │
│  │  • ClientServiceImpl   │  • JSON Serialization       │   │
│  │  • Streaming Support  │  • ORM Integration          │   │
│  └──────────┬────────────┴───────────┬──────────────────┘   │
│             │                        │                       │
│  ┌──────────┴────────────┬───────────┴──────────────────┐   │
│  │       Service Layer (Core Business Logic)           │   │
│  ├─────────────────────────────────────────────────────┤   │
│  │  • RunService          • AuthService                │   │
│  │  • LogService          • OtaService                 │   │
│  │  • ResultService       • AlertService               │   │
│  │  • NotificationService • SchedulerService           │   │
│  └──────────┬────────────────────────┬──────────────────┘   │
│             │                        │                       │
│  ┌──────────┴────────────┬───────────┴──────────────────┐   │
│  │     Repository & ORM Layer (Data Access)            │   │
│  ├─────────────────────────────────────────────────────┤   │
│  │  • Drogon ORM Mappers                               │   │
│  │  • SQL Query Builders                               │   │
│  │  • Connection Pooling                               │   │
│  │  • Migration Runner                                 │   │
│  └──────────┬────────────────────────┬──────────────────┘   │
│             │                        │                       │
│             └────────────┬───────────┘                       │
│                          │                                   │
│  ┌──────────────────────┴──────────────────────┐            │
│  │    PostgreSQL 15+ Database                  │            │
│  │  • Versioned Schema                         │            │
│  │  • Audit Logs                               │            │
│  │  • Test Results & Metadata                  │            │
│  │  • Configuration & Settings                 │            │
│  │  • User Sessions & Authentication           │            │
│  └─────────────────────────────────────────────┘            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## System Requirements

### Minimum Requirements

| Component | Version | Reason |
|-----------|---------|--------|
| Ubuntu | 22.04 LTS+ | Stable, well-tested base |
| CMake | 3.22+ | C++20 module support |
| GCC | 12+ | C++20 coroutines, modules |
| Memory | 4 GB RAM | Compilation, Docker daemon |
| Disk | 3-5 GB free | Source builds (Drogon, etc.) |
| CPU | 2+ cores | Parallel compilation |

### Recommended Environment

- **OS**: Ubuntu 24.04 LTS (newer packages)
- **CPU**: 4+ cores (faster builds)
- **RAM**: 8 GB+ (smoother experience)
- **Disk**: 10 GB+ free SSD (faster I/O)

---

## Dependencies Reference

This section documents every dependency, its version, source, and purpose.

### 1. Build & Development Tools

| Package | Version | Purpose | Installed Via |
|---------|---------|---------|---------------|
| `build-essential` | system | GCC, g++, make | apt-get |
| `cmake` | 3.22+ | Build system | apt-get |
| `ninja-build` | system | Fast build backend | apt-get |
| `git` | system | Version control | apt-get |
| `pkg-config` | system | Library discovery | apt-get |
| `wget` | system | HTTP downloads | apt-get |
| `ccache` | system | Compiler result caching | apt-get |

**Why these?**
- `build-essential`: Provides the compiler toolchain (GCC 12+ on Ubuntu 22.04)
- `cmake`: Configures the project based on platform and dependencies
- `ninja-build`: Significantly faster builds than Make (optional but recommended)
- `pkg-config`: Discovers library paths and flags at compile time
- `ccache`: Speeds up rebuilds by caching compilation results

---

### 2. Protocol Buffers & gRPC

| Package | Version | Purpose | Installed Via |
|---------|---------|---------|---------------|
| `protobuf-compiler` | 3.x | protoc binary | apt-get |
| `libprotobuf-dev` | 3.x | Protobuf C++ library | apt-get |
| `libprotoc-dev` | 3.x | Protocol compiler library | apt-get |
| `libgrpc-dev` | 1.x | gRPC core library | apt-get |
| `libgrpc++-dev` | 1.x | gRPC C++ bindings | apt-get |
| `grpc_cpp_plugin` | 1.30.2 | Protobuf plugin for gRPC | source build |

**Why these?**
- **Protocol Buffers**: Serialization format for client-server communication
  - Compact binary format: ~3x smaller than JSON
  - Schema versioning: Backward/forward compatible
  - Code generation: Auto-generated C++ classes from `.proto` files
- **gRPC**: High-performance RPC framework
  - Bidirectional streaming: Essential for real-time test result streams
  - HTTP/2 multiplexing: Multiple concurrent requests on single connection
  - gRPC Reflection: Allows dynamic service discovery

**Generated Protos** (from `/workspace/Zion-HAL/protos/`):
```
├── common.proto              → TestResult, RunCommand, etc.
├── hal_service.proto         → HAL device communication
└── client_service.proto      → Embedded client ↔ server protocol
```

---

### 3. Web Framework & REST API

| Package | Version | Purpose | Installed Via |
|---------|---------|---------|---------------|
| `drogon` | 1.9.3 | C++ web framework | source build |
| `libjsoncpp-dev` | system | JSON library | apt-get |
| `uuid-dev` | system | UUID generation | apt-get |
| `zlib1g-dev` | system | HTTP compression | apt-get |
| `libbrotli-dev` | system | Brotli compression | apt-get |
| `libc-ares-dev` | system | Async DNS | apt-get |

**Why Drogon 1.9.3?**
- **Lightweight ORM**: Direct PostgreSQL integration (no external migration tools)
- **Zero-copy**: Efficient request handling with minimal allocations
- **Built-in middleware**: Authentication, CORS, request logging
- **OpenAPI support**: Auto-generates API documentation
- **No external dependencies**: Fully self-contained (unlike larger frameworks)

**Drogon Compilation** (takes 5-10 min):
```
cmake -DUSE_POSTGRESQL=ON -DUSE_MYSQL=OFF -DUSE_SQLITE3=OFF
```

---

### 4. TLS/Security

| Package | Version | Purpose | Installed Via |
|---------|---------|---------|---------------|
| `libssl-dev` | 3.x | OpenSSL library | apt-get |
| `libargon2-dev` | system | Password hashing | apt-get |

**Why these?**
- **OpenSSL**: HTTPS support, certificate handling for gRPC TLS
- **Argon2**: Memory-hard password hashing (OWASP recommended)

---

### 5. Logging & Testing

| Package | Version | Purpose | Installed Via |
|---------|---------|---------|---------------|
| `libspdlog-dev` | 1.x | High-performance logging | apt-get |
| `libgtest-dev` | system | Google Test framework | apt-get |

**Why these?**
- **spdlog**: Header-only, zero-allocation logging (critical for performance)
- **GTest**: Industry-standard C++ testing framework

---

### 6. YAML Configuration

| Package | Version | Purpose | Installed Via |
|---------|---------|---------|---------------|
| `yaml-cpp` | 0.8.0 | YAML parsing/generation | source build |

**Why build from source?**
- Ubuntu 22.04 ships 0.7.0 which lacks the `yaml-cpp::yaml-cpp` CMake target
- 0.8.0 adds proper CMake exports and better C++ standards compliance

```bash
# Built from: https://github.com/jbeder/yaml-cpp/releases/tag/0.8.0
```

---

### 7. Database

| Package | Version | Purpose | Installed Via |
|---------|---------|---------|---------------|
| `libpq-dev` | system | PostgreSQL C client | apt-get |
| `postgres` | 15+ | Database server | Docker |

**Why PostgreSQL?**
- **ACID guarantees**: Data consistency for test results
- **JSON support**: Flexible schema for test metadata
- **Full-text search**: Search logs and results
- **Versioning**: Migration system for schema evolution

**Connection**: Uses Drogon ORM (built-in connection pooling)

---

### 8. Optional: Prometheus Metrics

| Package | Version | Purpose | Installed Via |
|---------|---------|---------|---------------|
| `prometheus-cpp` | — | Metrics exposition | vcpkg (if enabled) |

**Purpose**: 
- Exposes metrics at `/metrics` endpoint (Prometheus format)
- Tracks request counts, latencies, error rates
- Completely optional (CMake checks if available)

---

## Installation Instructions

### Quick Start

```bash
# Run the automated installation script
cd /path/to/Zion-Server
./local_install.sh
```

**Installation time**: 10-15 minutes (including builds from source)

### Step-by-Step Manual Installation

#### Step 1: Update System

```bash
sudo apt-get update
sudo apt-get upgrade -y
```

#### Step 2: Install System Packages

```bash
sudo apt-get install -y \
  build-essential cmake ninja-build git pkg-config wget curl ccache sudo \
  protobuf-compiler libprotobuf-dev libprotoc-dev \
  libgrpc++-dev libgrpc-dev \
  libssl-dev \
  libjsoncpp-dev uuid-dev zlib1g-dev libbrotli-dev libc-ares-dev \
  libpq-dev \
  libgtest-dev libspdlog-dev libargon2-dev
```

#### Step 3: Build Dependencies from Source

##### yaml-cpp 0.8.0

```bash
git clone --depth=1 --branch 0.8.0 https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DYAML_CPP_BUILD_TESTS=OFF
cmake --build build --parallel $(nproc)
sudo cmake --install build
sudo ldconfig
cd ..
```

##### Drogon v1.9.3

```bash
git clone --depth=1 --branch v1.9.3 https://github.com/drogonframework/drogon.git
cd drogon
git submodule update --init --recursive
cmake -S . -B build \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DBUILD_CTL=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTING=OFF \
  -DUSE_POSTGRESQL=ON \
  -DUSE_MYSQL=OFF \
  -DUSE_SQLITE3=OFF
cmake --build build --parallel $(nproc)
sudo cmake --install build
sudo ldconfig
cd ..
```

##### grpc_cpp_plugin v1.30.2

```bash
mkdir -p grpc_plugin/src/compiler
cd grpc_plugin
# Download compiler source files
BASE="https://raw.githubusercontent.com/grpc/grpc/v1.30.2/src/compiler"
for f in config.h config_protobuf.h generator_helpers.h schema_interface.h \
         protobuf_plugin.h cpp_generator.h cpp_generator.cc \
         cpp_generator_helpers.h cpp_plugin.h cpp_plugin.cc; do
  wget "${BASE}/${f}" -O "src/compiler/${f}"
done

# Compile
g++ -std=c++14 -O2 -o grpc_cpp_plugin \
  src/compiler/cpp_plugin.cc src/compiler/cpp_generator.cc -I. \
  $(pkg-config --cflags protobuf) -lprotoc -lprotobuf

# Install
sudo install -m 0755 grpc_cpp_plugin /usr/local/bin/
cd ..
```

#### Step 4: Verify Installation

```bash
cmake --version
protoc --version
grpc_cpp_plugin --version
pkg-config --list-all | grep -E "grpc|protobuf|yaml|spdlog"
```

---

## Post-Installation Setup

### 1. Source Environment Variables

```bash
source /path/to/Zion-Server/.env.local
```

This sets:
- `CMAKE_PREFIX_PATH` for custom libraries
- `PKG_CONFIG_PATH` for library discovery
- `LD_LIBRARY_PATH` for runtime linking
- C/C++ compiler flags

### 2. Start PostgreSQL

Either via Docker (recommended):
```bash
cd docker
docker compose -f docker-compose.yml up postgres -d
```

Or install locally:
```bash
sudo apt-get install postgresql postgresql-contrib
sudo systemctl start postgresql
```

### 3. Initialize Database

```bash
psql -U postgres -c "CREATE DATABASE zion;"
psql -U zion zion < migrations/0001_init.sql
# ... run remaining migrations
```

Or let the server auto-migrate:
```bash
cd /path/to/Zion-Server
cmake --preset debug
cmake --build build/debug
./build/debug/bin/zion-server  # Auto-runs migrations on startup
```

---

## Troubleshooting

### CMake Can't Find gRPC

**Error**: `Could not find gRPC_gRPC_PACKAGE`

**Solutions**:
1. Verify grpc is installed: `pkg-config --cflags grpc++`
2. Check CMake path: `export CMAKE_PREFIX_PATH="/usr/local/lib/cmake:$CMAKE_PREFIX_PATH"`
3. If using system libgrpc-dev, manually copy CMake files:
   ```bash
   sudo mkdir -p /usr/lib/x86_64-linux-gnu/cmake/grpc
   # Copy gRPCConfig.cmake and gRPCConfigVersion.cmake from docker/
   ```

### protoc or grpc_cpp_plugin Not Found

**Error**: `protoc: command not found` or `grpc_cpp_plugin: command not found`

**Solutions**:
```bash
# Verify installation
which protoc
which grpc_cpp_plugin

# If not found, add to PATH
export PATH="/usr/local/bin:$PATH"

# Or rebuild from source
./local_install.sh --skip-apt
```

### yaml-cpp Library Not Found

**Error**: `could not find yaml-cpp`

**Solutions**:
1. Verify it's installed: `pkg-config --cflags yaml-cpp`
2. Update library cache: `sudo ldconfig`
3. Rebuild: `./local_install.sh --skip-apt`

### Drogon Build Fails

**Error**: `error: [plugin]: libprotoc.a: No such file or directory`

**Solutions**:
1. Ensure libprotoc-dev is installed: `apt-get install libprotoc-dev`
2. Check pkg-config: `pkg-config --cflags protobuf`
3. Try again: `./local_install.sh --skip-apt`

### PostgreSQL Connection Fails

**Error**: `could not connect to server: Connection refused`

**Solutions**:
```bash
# Check if PostgreSQL is running
docker compose -f docker/docker-compose.yml ps postgres

# Or locally
sudo systemctl status postgresql

# Check credentials in .env
grep ZION_DB .env

# Test connection
psql -h localhost -U zion -d zion -c "SELECT 1"
```

### Build Takes Too Long

**Solutions**:
1. Use `ninja` instead of `make`:
   ```bash
   cmake --preset debug -G Ninja
   cmake --build build/debug
   ```

2. Enable ccache:
   ```bash
   export PATH="/usr/lib/ccache:$PATH"
   cmake --preset debug
   ```

3. Reduce parallel jobs:
   ```bash
   cmake --build build/debug --parallel 2
   ```

### Out of Memory During Build

**Solutions**:
1. Reduce parallel jobs:
   ```bash
   cmake --build build/debug --parallel 1
   ```

2. Build Drogon first, separately:
   ```bash
   # Just let the script build it, don't try parallel
   ```

3. Increase swap:
   ```bash
   sudo dd if=/dev/zero of=/swapfile bs=1G count=2
   sudo chmod 600 /swapfile
   sudo mkswap /swapfile
   sudo swapon /swapfile
   ```

---

## Development Workflow

### 1. Build for Development (with Debug Info)

```bash
cd /path/to/Zion-Server
cmake --preset debug
cmake --build build/debug
```

**Presets** (defined in `CMakePresets.json`):
- `debug`: RelWithDebInfo + sanitizers
- `release`: Optimized binary
- `coverage`: Debug + code coverage

### 2. Running the Server

```bash
# With auto-loaded .env
./build/debug/bin/zion-server

# Or manually
ZION_DB_HOST=localhost \
ZION_DB_PORT=5432 \
ZION_DB_NAME=zion \
ZION_DB_USER=zion \
ZION_DB_PASSWORD=zion_dev \
ZION_PORT=8080 \
ZION_GRPC_PORT=9090 \
./build/debug/bin/zion-server
```

### 3. Running Tests

```bash
# All tests
ctest --test-dir build/debug --output-on-failure

# Unit tests only
ctest --test-dir build/debug -L "!integration" --output-on-failure

# Integration tests only
ctest --test-dir build/debug -L "integration" --output-on-failure

# Specific test
ctest --test-dir build/debug -R "TestName" --output-on-failure
```

### 4. Running with Docker (Recommended for Development)

```bash
cd docker
./setup.sh
docker exec -it zion-dev bash
cmake --preset debug
cmake --build build/debug
ctest --test-dir build/debug
```

### 5. Code Quality

```bash
# Format code (if clang-format is configured)
find src -name "*.cc" -o -name "*.h" | xargs clang-format -i

# Run static analysis (optional)
cppcheck --enable=all src/

# Check compilation warnings
cmake --build build/debug 2>&1 | grep warning
```

---

## Dependency Update Notes

### When to Update Dependencies

| Dependency | Stability | When to Update |
|-----------|-----------|----------------|
| protobuf | ⭐⭐⭐⭐⭐ | Rarely (breaking changes) |
| gRPC | ⭐⭐⭐⭐⭐ | Annually (or for fixes) |
| Drogon | ⭐⭐⭐⭐ | Major version changes |
| yaml-cpp | ⭐⭐⭐⭐ | Minor patches only |
| spdlog | ⭐⭐⭐⭐⭐ | After 1-2 patch releases |
| PostgreSQL | ⭐⭐⭐⭐⭐ | LTS versions only |

### Updating a Dependency

Example: Update Drogon to v1.10.0

```bash
# Update CMakeLists.txt and Dockerfile.server
# Build in temporary directory
git clone --depth=1 --branch v1.10.0 \
  https://github.com/drogonframework/drogon.git /tmp/drogon-new
cd /tmp/drogon-new
# ... build & test ...
sudo cmake --install build  # Replace old installation

# Run full regression tests
ctest --test-dir build/debug --output-on-failure
```

---

## Architecture Rationale

### Why C++20?

- **Coroutines**: Async I/O without callbacks (`co_await`)
- **Modules**: Faster compilation than headers
- **Concepts**: Type constraints for generic code
- **Ranges**: Modern iteration over collections

### Why gRPC + REST?

- **gRPC**: High-performance, streaming for real-time data
- **REST**: Easy integration with web UIs, standard tooling

### Why Drogon?

- **Lightweight**: No external runtime dependencies
- **Fast**: Zero-copy message handling
- **ORM**: Direct PostgreSQL without separate libraries
- **Self-contained**: Don't need Node.js/Python/Java server

### Why PostgreSQL?

- **ACID**: Reliable data consistency
- **JSON**: Flexible schema for metadata
- **Mature**: 25+ years of stability
- **Scalable**: Handles millions of test results

---

## Further Reading

- [CMake Documentation](https://cmake.org/documentation/)
- [gRPC C++ Guide](https://grpc.io/docs/languages/cpp/)
- [Drogon Framework](https://github.com/drogonframework/drogon)
- [Protocol Buffers Documentation](https://developers.google.com/protocol-buffers)
- [PostgreSQL Documentation](https://www.postgresql.org/docs/)
- [spdlog GitHub](https://github.com/gabime/spdlog)

---

## Support & Contribution

For issues or questions:
1. Check this documentation
2. Review Docker setup in `docker/README.md`
3. Check GitHub Issues
4. Create detailed issue with logs and environment info

---

**Last Updated**: 2024-12
**Target Platforms**: Ubuntu 22.04 LTS, Ubuntu 24.04 LTS
**Maintainers**: Zion Development Team
