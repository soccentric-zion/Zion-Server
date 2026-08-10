# Zion Server — Dependency Summary Quick reference guide for all dependencies used in the Zion Server project. ## System Packages via apt-get ### Build Tools
```bash
build-essential # GCC, g++, make
cmake # Build system (≥3.22)
ninja-build # Fast build backend
git # Version control
pkg-config # Library discovery
wget # File downloads
curl # HTTP client
ccache # Compiler caching
``` ### Protobuf & gRPC
```bash
protobuf-compiler # protoc compiler
libprotobuf-dev # Protobuf C++ library
libprotoc-dev # Protobuf compiler library
libgrpc-dev # gRPC core
libgrpc++-dev # gRPC C++
``` ### Security & Crypto
```bash
libssl-dev # OpenSSL
libargon2-dev # Password hashing
``` ### Web Framework Dependencies
```bash
libjsoncpp-dev # JSON parsing
uuid-dev # UUID generation
zlib1g-dev # Compression
libbrotli-dev # Brotli compression
libc-ares-dev # Async DNS
libpq-dev # PostgreSQL client
``` ### Testing & Logging
```bash
libgtest-dev # Google Test
libspdlog-dev # spdlog logging
``` ## Dependencies Built from Source ### 1. yaml-cpp 0.8.0
- **Source**: https://github.com/jbeder/yaml-cpp
- **Reason**: Ubuntu 22.04 ships 0.7.0 (missing CMake targets)
- **CMake Flags**: ```cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DYAML_CPP_BUILD_TESTS=OFF ```
- **Build Time**: ~30 seconds
- **Size**: ~5 MB ### 2. Drogon v1.9.3
- **Source**: https://github.com/drogonframework/drogon
- **Reason**: Not packaged for Ubuntu 22.04
- **CMake Flags**: ```cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_CTL=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DUSE_POSTGRESQL=ON -DUSE_MYSQL=OFF -DUSE_SQLITE3=OFF ```
- **Build Time**: 5-10 minutes (depends on CPU)
- **Size**: ~50 MB
- **Includes Submodules**: json, nlohmann, trantor ### 3. grpc_cpp_plugin v1.30.2
- **Source**: https://github.com/grpc/grpc (src/compiler)
- **Reason**: Ubuntu's libgrpc-dev has no binary plugin
- **Build**: Compile from v1.30.2 source files
- **Build Time**: ~1 minute
- **Installed To**: `/usr/local/bin/grpc_cpp_plugin` ## Optional Dependencies ### Prometheus Metrics (Optional)
```bash
prometheus-cpp # Metrics exposition
```
- Enabled via: `find_package(prometheus-cpp CONFIG QUIET)`
- Compiled with: `ZION_HAS_PROMETHEUS=1` if found ## Runtime Dependencies (Not Required for Build) ### PostgreSQL 15+
- **Run via**: Docker: `docker compose up postgres`
- **Or**: `sudo apt install postgresql`
- **Port**: 5432 (default)
- **Connection**: Drogon ORM handles connection pooling ### Docker (For containerized development)
- Recommended for PostgreSQL and isolated dev environment
- See: `docker/docker-compose.yml` ## Dependency Versions | Component | Version | Constraint |
|-----------|---------|-----------|
| CMake | 3.22+ | Exact |
| GCC | 12+ | Exact |
| C++ Standard | C++20 | Exact |
| Protocol Buffers | 3.x | Exact |
| gRPC | 1.30.2 | Exact (for plugin) |
| Drogon | 1.9.3 | Exact |
| yaml-cpp | 0.8.0 | Exact |
| PostgreSQL | 15+ | Minimum |
| OpenSSL | 3.x | Minimum |
| spdlog | 1.x | Minimum |
| Google Test | Latest | Minimum | ## Installation Commands ### Automatic (Recommended)
```bash
./local_install.sh
``` ### Manual - Install All at Once
```bash
# System packages
sudo apt-get update
sudo apt-get install -y \ build-essential cmake ninja-build git pkg-config wget curl ccache \ protobuf-compiler libprotobuf-dev libprotoc-dev \ libgrpc++-dev libgrpc-dev \ libssl-dev \ libjsoncpp-dev uuid-dev zlib1g-dev libbrotli-dev libc-ares-dev libpq-dev \ libgtest-dev libspdlog-dev libargon2-dev # Then run local_install.sh to build from source
./local_install.sh --skip-apt
``` ## Dependency Graph ```
Zion Server (C++20)
├── Build System
│ ├── CMake 3.22+
│ └── Ninja (or Make)
├── Protocol & RPC
│ ├── protobuf 3.x
│ ├── gRPC 1.30.2
│ └── grpc_cpp_plugin 1.30.2
├── Web Framework
│ ├── Drogon 1.9.3
│ │ ├── jsoncpp
│ │ ├── libuuid
│ │ ├── zlib
│ │ ├── libbrotli
│ │ ├── c-ares
│ │ └── PostgreSQL client
│ └── OpenSSL 3.x
├── Data & Configuration
│ ├── PostgreSQL 15+
│ └── yaml-cpp 0.8.0
├── Utilities
│ ├── spdlog 1.x (logging)
│ └── Argon2 (password hashing)
├── Testing
│ └── Google Test
└── Optional └── prometheus-cpp (metrics)
``` ## Library File Locations After installation, libraries are located at: ```bash
# System libraries/usr/lib/x86_64-linux-gnu//usr/include/ # Custom installations/usr/local/lib/ # libdrogon.a, libyaml-cpp.a, etc./usr/local/include/ # drogon, yaml-cpp headers/usr/local/bin/ # grpc_cpp_plugin, protoc, etc. # CMake modules/usr/local/lib/cmake//usr/lib/x86_64-linux-gnu/cmake/ # pkg-config/usr/local/lib/pkgconfig//usr/lib/x86_64-linux-gnu/pkgconfig/
``` ## Verification Commands ```bash
# Check cmake
cmake --version # Check compiler
g++ --version # Check protobuf
protoc --version # Check gRPC plugin
grpc_cpp_plugin --version # Check libraries with pkg-config
pkg-config --list-all | grep -E "grpc|protobuf|yaml|drogon|spdlog|openssl" # Check PostgreSQL
psql --version # Verify installations
file/usr/local/lib/libdrogon.a
ls -la/usr/local/include/drogon
ls -la/usr/local/lib/cmake/
``` ## Storage Requirements | Component | Size | Reason |
|-----------|------|--------|
| System packages | ~500 MB | Build tools, libraries, headers |
| yaml-cpp source | ~5 MB | Temporary (cleaned up) |
| Drogon source | ~50 MB | Temporary (cleaned up) |
| grpc build files | ~20 MB | Temporary (cleaned up) |
| **Total on Disk** | ~2-3 GB | For full installation | **Note**: The `local_install.sh` script cleans up temporary files after installation, so final disk usage is ~500MB-1GB for all libraries. ## Build Time Estimates | Task | Time |
|------|------|
| apt-get update + install | 2-3 min |
| yaml-cpp build | ~30 sec |
| Drogon build | 5-10 min |
| grpc_cpp_plugin build | ~1 min |
| **Total** | **10-15 min** | *Times vary based on CPU cores and internet speed* ## Architecture Support | Architecture | Status | Notes |
|-------------|--------|-------|
| x86_64 | ✅ Fully supported | Recommended |
| ARM64 (aarch64) | ✅ Fully supported | Zynq UltraScale+, M1/M2 Mac |
| armv7l | ⚠️ Untested | May work with GCC 12+ |
| PowerPC | ❌ Not supported | C++20 compiler missing | ## Compiler Support | Compiler | Version | Status |
|----------|---------|--------|
| GCC | 12+ | ✅ Recommended |
| GCC | 11 | ⚠️ May work (C++20 incomplete) |
| Clang | 16+ | ✅ Works |
| MSVC | 2022+ | ⚠️ Untested |
| Intel icc | — | ❌ Not tested | ## Environment Variables After installation, source `.env.local`: ```bash
source/path/to/Zion-Server/.env.local
``` Sets:
```bash
CMAKE_PREFIX_PATH="/usr/local/lib/cmake:${CMAKE_PREFIX_PATH}"
CMAKE_LIBRARY_PATH="/usr/local/lib:${CMAKE_LIBRARY_PATH}"
CMAKE_INCLUDE_PATH="/usr/local/include:${CMAKE_INCLUDE_PATH}"
PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:${PKG_CONFIG_PATH}"
LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"
``` ## Troubleshooting Checklist - [ ] Ubuntu 22.04 or later? `cat/etc/os-release`
- [ ] CMake 3.22+? `cmake --version`
- [ ] GCC 12+? `gcc --version`
- [ ] libprotobuf-dev installed? `pkg-config --cflags protobuf`
- [ ] grpc_cpp_plugin in PATH? `which grpc_cpp_plugin`
- [ ] yaml-cpp installed? `pkg-config --cflags yaml-cpp`
- [ ] Drogon installed? `pkg-config --cflags drogon`
- [ ] Environment sourced? `source .env.local`
- [ ] PostgreSQL running? `docker compose up postgres &` --- For detailed setup instructions, see: [LOCAL_SETUP.md](LOCAL_SETUP.md)
