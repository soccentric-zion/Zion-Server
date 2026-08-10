#!/bin/bash
################################################################################
# Zion Server — Local Development Environment Setup
#
# This script installs all build dependencies and development tools needed to
# build the Zion Server project from source on a local machine (Ubuntu 22.04+).
#
# Supports: x86_64 and ARM64 architectures
# Tested on: Ubuntu 22.04 LTS, Ubuntu 24.04 LTS
#
# Usage: ./local_install.sh [--help] [--skip-apt] [--skip-postgres]
################################################################################

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
TOTAL_STEPS=0
CURRENT_STEP=0

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"

# Build directories
BUILD_TEMP_DIR="${PROJECT_ROOT}/.local_build_temp"

################################################################################
# Helper Functions
################################################################################

print_header() {
    echo ""
    echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
    echo ""
}

print_step() {
    CURRENT_STEP=$((CURRENT_STEP + 1))
    echo -e "${BLUE}[${CURRENT_STEP}/${TOTAL_STEPS}]${NC} ${YELLOW}$1${NC}"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

check_command() {
    if command -v "$1" &> /dev/null; then
        return 0
    else
        return 1
    fi
}

show_help() {
    cat << 'EOF'
Zion Server — Local Development Environment Setup

USAGE:
    ./local_install.sh [OPTIONS]

OPTIONS:
    --skip-apt              Skip system package installation via apt
    --skip-postgres         Skip PostgreSQL client library installation
    --skip-build-from-src   Skip building dependencies from source (yaml-cpp, drogon, grpc_cpp_plugin)
    --help                  Show this help message

EXAMPLES:
    # Full installation
    ./local_install.sh

    # Skip PostgreSQL (if you only need build tools)
    ./local_install.sh --skip-postgres

    # Skip apt installation (dependencies already installed)
    ./local_install.sh --skip-apt

REQUIREMENTS:
    • Ubuntu 22.04 LTS or later
    • sudo access (for apt and make install)
    • Internet connection
    • ~2-3 GB free disk space (for source builds)

BUILD TIME:
    • With full build-from-source: ~10-15 minutes
    • With pre-built packages: ~3-5 minutes

POST-INSTALLATION:
    1. Set up environment variables:
       export VCPKG_ROOT=/path/to/vcpkg  (if using vcpkg)

    2. Verify installation:
       cmake --version
       protoc --version
       grpc_cpp_plugin --version

    3. Build the project:
       cd /path/to/Zion-Server
       cmake --preset debug
       cmake --build build/debug

    4. Run tests:
       ctest --test-dir build/debug --output-on-failure

    5. Start PostgreSQL (in separate terminal):
       docker compose -f docker/docker-compose.yml up postgres

EOF
}

################################################################################
# Pre-flight Checks
################################################################################

verify_prerequisites() {
    print_step "Verifying system prerequisites"

    # Check if running on Linux
    if [[ "$OSTYPE" != "linux-gnu"* ]]; then
        print_error "This script only supports Linux systems"
        exit 1
    fi

    # Check if running on Ubuntu/Debian
    if ! check_command apt-get; then
        print_error "apt-get not found. This script requires Debian/Ubuntu"
        exit 1
    fi

    # Check Ubuntu version (22.04+)
    if ! grep -q "jammy\|noble" /etc/os-release 2>/dev/null; then
        print_warning "This script is optimized for Ubuntu 22.04 or 24.04"
    fi

    # Check for sudo access
    if ! sudo -n true 2>/dev/null; then
        print_warning "Will need sudo access. You may be prompted for password."
    fi

    # Check disk space (require ~2GB free)
    AVAILABLE_SPACE=$(df "$PROJECT_ROOT" | tail -1 | awk '{print $4}')
    if [ "$AVAILABLE_SPACE" -lt 2097152 ]; then  # 2GB in KB
        print_warning "Less than 2GB free disk space. Build from source may fail."
    fi

    print_success "System prerequisites verified"
}

################################################################################
# APT Package Installation
################################################################################

install_apt_packages() {
    print_step "Installing system packages via apt"

    # Update package lists
    print_info "Updating package lists..."
    sudo apt-get update -qq

    # Core build tools
    local BUILD_TOOLS=(
        "build-essential"           # GCC, g++, make
        "cmake"                     # CMake build system
        "ninja-build"               # Ninja build backend
        "git"                       # Version control
        "pkg-config"                # Library metadata
        "wget"                      # Download utility
        "curl"                      # HTTP client
        "ccache"                    # Compiler cache
        "sudo"                      # Privilege escalation
    )

    # Protobuf & gRPC
    local GRPC_TOOLS=(
        "protobuf-compiler"         # protoc
        "libprotobuf-dev"           # protobuf headers & lib
        "libprotoc-dev"             # protoc development library
        "libgrpc++-dev"             # gRPC C++ library
        "libgrpc-dev"               # gRPC core library
    )

    # TLS/SSL
    local SECURITY_LIBS=(
        "libssl-dev"                # OpenSSL development
    )

    # Drogon dependencies
    local DROGON_DEPS=(
        "libjsoncpp-dev"            # JSON parsing
        "uuid-dev"                  # UUID generation
        "zlib1g-dev"                # Compression
        "libbrotli-dev"             # Brotli compression
        "libc-ares-dev"             # C-Ares DNS resolution
    )

    # PostgreSQL
    local POSTGRES_DEPS=(
        "libpq-dev"                 # PostgreSQL client library
    )

    # Testing & utilities
    local TEST_TOOLS=(
        "libgtest-dev"              # Google Test framework
        "libspdlog-dev"             # spdlog logging library
        "libargon2-dev"             # Argon2 password hashing
    )

    # Combine all packages
    local ALL_PACKAGES=(
        "${BUILD_TOOLS[@]}"
        "${GRPC_TOOLS[@]}"
        "${SECURITY_LIBS[@]}"
        "${DROGON_DEPS[@]}"
        "${POSTGRES_DEPS[@]}"
        "${TEST_TOOLS[@]}"
    )

    print_info "Installing ${#ALL_PACKAGES[@]} packages..."
    sudo apt-get install -y "${ALL_PACKAGES[@]}" 2>&1 | grep -E "^(Setting up|Processing|Reading)" || true

    # Verify key packages
    local CRITICAL_PACKAGES=("cmake" "protobuf-compiler" "libgrpc++-dev" "libssl-dev")
    for pkg in "${CRITICAL_PACKAGES[@]}"; do
        if dpkg -l | grep -q "^ii.*$pkg"; then
            print_success "Package installed: $pkg"
        else
            print_error "Failed to install critical package: $pkg"
            exit 1
        fi
    done
}

################################################################################
# Build from Source
################################################################################

build_yaml_cpp() {
    print_step "Building yaml-cpp 0.8.0 from source"

    local YAML_BUILD_DIR="${BUILD_TEMP_DIR}/yaml-cpp"
    
    if [ -d "$YAML_BUILD_DIR" ]; then
        print_info "Removing previous yaml-cpp build..."
        rm -rf "$YAML_BUILD_DIR"
    fi

    mkdir -p "$YAML_BUILD_DIR"
    cd "$YAML_BUILD_DIR"

    print_info "Cloning yaml-cpp v0.8.0..."
    git clone --depth=1 --branch 0.8.0 https://github.com/jbeder/yaml-cpp.git .

    print_info "Building yaml-cpp..."
    cmake -S . -B build \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DYAML_CPP_BUILD_TESTS=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -G Ninja

    cmake --build build --parallel "$(nproc)"

    print_info "Installing yaml-cpp..."
    sudo cmake --install build

    # Update library cache
    sudo ldconfig

    print_success "yaml-cpp 0.8.0 installed"
    cd "$PROJECT_ROOT"
}

build_drogon() {
    print_step "Building Drogon v1.9.3 from source"

    local DROGON_BUILD_DIR="${BUILD_TEMP_DIR}/drogon"
    
    if [ -d "$DROGON_BUILD_DIR" ]; then
        print_info "Removing previous Drogon build..."
        rm -rf "$DROGON_BUILD_DIR"
    fi

    mkdir -p "$DROGON_BUILD_DIR"
    cd "$DROGON_BUILD_DIR"

    print_info "Cloning Drogon v1.9.3..."
    git clone --depth=1 --branch v1.9.3 https://github.com/drogonframework/drogon.git .
    git submodule update --init --recursive

    print_info "Building Drogon (this may take 5-10 minutes)..."
    cmake -S . -B build \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_CTL=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF \
        -DUSE_POSTGRESQL=ON \
        -DUSE_MYSQL=OFF \
        -DUSE_SQLITE3=OFF \
        -G Ninja

    cmake --build build --parallel "$(nproc)"

    print_info "Installing Drogon..."
    sudo cmake --install build

    # Update library cache
    sudo ldconfig

    print_success "Drogon v1.9.3 installed"
    cd "$PROJECT_ROOT"
}

build_grpc_cpp_plugin() {
    print_step "Building grpc_cpp_plugin from source"

    local GRPC_BUILD_DIR="${BUILD_TEMP_DIR}/grpc"
    
    if [ -d "$GRPC_BUILD_DIR" ]; then
        print_info "Removing previous grpc build..."
        rm -rf "$GRPC_BUILD_DIR"
    fi

    mkdir -p "$GRPC_BUILD_DIR/src/compiler"
    cd "$GRPC_BUILD_DIR"

    print_info "Downloading gRPC compiler source files (v1.30.2)..."
    local BASE="https://raw.githubusercontent.com/grpc/grpc/v1.30.2/src/compiler"
    local FILES=(
        "config.h" "config_protobuf.h" "generator_helpers.h" "schema_interface.h"
        "protobuf_plugin.h"
        "cpp_generator.h" "cpp_generator.cc" "cpp_generator_helpers.h"
        "cpp_plugin.h" "cpp_plugin.cc"
    )

    for file in "${FILES[@]}"; do
        print_info "Downloading src/compiler/$file..."
        if ! wget -q "${BASE}/${file}" -O "src/compiler/${file}"; then
            print_error "Failed to download ${file}"
            return 1
        fi
    done

    print_info "Compiling grpc_cpp_plugin..."
    g++ -std=c++14 -O2 \
        -o grpc_cpp_plugin \
        src/compiler/cpp_plugin.cc \
        src/compiler/cpp_generator.cc \
        -I. \
        $(pkg-config --cflags protobuf) \
        -lprotoc -lprotobuf

    print_info "Installing grpc_cpp_plugin..."
    sudo install -m 0755 grpc_cpp_plugin /usr/local/bin/grpc_cpp_plugin

    print_success "grpc_cpp_plugin installed at /usr/local/bin/grpc_cpp_plugin"
    cd "$PROJECT_ROOT"
}

################################################################################
# Configuration
################################################################################

configure_cmake() {
    print_step "Setting up CMake paths"

    # Create /usr/local/lib/cmake directories if they don't exist
    sudo mkdir -p /usr/local/lib/cmake/grpc

    # Check if gRPC cmake configs exist in system location
    if [ ! -f "/usr/lib/x86_64-linux-gnu/cmake/grpc/gRPCConfig.cmake" ] && \
       [ ! -f "/usr/local/lib/cmake/grpc/gRPCConfig.cmake" ]; then
        print_warning "gRPC CMake config not found, may need manual setup"
        print_info "If you encounter 'find_package(gRPC)' errors, ensure grpc CMake files are in:"
        print_info "  /usr/lib/x86_64-linux-gnu/cmake/grpc/"
        print_info "  or /usr/local/lib/cmake/grpc/"
    else
        print_success "gRPC CMake config found"
    fi

    print_success "CMake paths configured"
}

setup_environment() {
    print_step "Setting up environment variables"

    # Create a shell snippet that users can source
    local ENV_FILE="${PROJECT_ROOT}/.env.local"

    cat > "$ENV_FILE" << 'EOF'
#!/bin/bash
# Zion Server — Local Development Environment

# C++ compiler
export CC=gcc
export CXX=g++

# CMake
export CMAKE_PREFIX_PATH="/usr/local/lib/cmake:${CMAKE_PREFIX_PATH}"
export CMAKE_LIBRARY_PATH="/usr/local/lib:${CMAKE_LIBRARY_PATH}"
export CMAKE_INCLUDE_PATH="/usr/local/include:${CMAKE_INCLUDE_PATH}"

# pkg-config
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:${PKG_CONFIG_PATH}"

# Library paths
export LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"

# Optional: use vcpkg if available
if [ -n "${VCPKG_ROOT:-}" ]; then
    export CMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
fi

echo "✓ Zion development environment loaded"
EOF

    chmod +x "$ENV_FILE"
    print_success "Environment file created: $ENV_FILE"
    print_info "Source it with: source ${ENV_FILE}"
}

################################################################################
# Verification
################################################################################

verify_installation() {
    print_step "Verifying installation"

    local FAILED=0

    # Check cmake
    if check_command cmake; then
        local CMAKE_VERSION=$(cmake --version | head -1)
        print_success "CMake: $CMAKE_VERSION"
    else
        print_error "CMake not found"
        FAILED=$((FAILED + 1))
    fi

    # Check protoc
    if check_command protoc; then
        local PROTOC_VERSION=$(protoc --version)
        print_success "protoc: $PROTOC_VERSION"
    else
        print_error "protoc not found"
        FAILED=$((FAILED + 1))
    fi

    # Check grpc_cpp_plugin
    if check_command grpc_cpp_plugin; then
        print_success "grpc_cpp_plugin: $(which grpc_cpp_plugin)"
    else
        print_error "grpc_cpp_plugin not found in PATH"
        FAILED=$((FAILED + 1))
    fi

    # Check pkg-config
    if check_command pkg-config; then
        print_success "pkg-config installed"
    else
        print_error "pkg-config not found"
        FAILED=$((FAILED + 1))
    fi

    # Check critical libraries
    local CRITICAL_LIBS=(
        "openssl"
        "libspdlog"
        "yaml-cpp"
        "grpc++"
        "protobuf"
    )

    for lib in "${CRITICAL_LIBS[@]}"; do
        if pkg-config --exists "$lib" 2>/dev/null || \
           pkg-config --exists "lib${lib}" 2>/dev/null; then
            print_success "Library found: $lib"
        else
            print_warning "Library not found via pkg-config: $lib"
        fi
    done

    if [ $FAILED -gt 0 ]; then
        print_warning "Some checks failed. Review the output above."
        return 1
    else
        print_success "All critical components verified"
        return 0
    fi
}

################################################################################
# Cleanup
################################################################################

cleanup_build_temp() {
    if [ -d "$BUILD_TEMP_DIR" ]; then
        print_info "Cleaning up temporary build files..."
        rm -rf "$BUILD_TEMP_DIR"
    fi
}

################################################################################
# Main Script
################################################################################

main() {
    # Parse command line arguments
    SKIP_APT=false
    SKIP_POSTGRES=false
    SKIP_BUILD_FROM_SRC=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            --help)
                show_help
                exit 0
                ;;
            --skip-apt)
                SKIP_APT=true
                shift
                ;;
            --skip-postgres)
                SKIP_POSTGRES=true
                shift
                ;;
            --skip-build-from-src)
                SKIP_BUILD_FROM_SRC=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done

    # Calculate total steps
    TOTAL_STEPS=6  # Default steps

    if [ "$SKIP_APT" = false ]; then
        TOTAL_STEPS=$((TOTAL_STEPS + 1))
    fi

    if [ "$SKIP_BUILD_FROM_SRC" = false ]; then
        TOTAL_STEPS=$((TOTAL_STEPS + 3))  # yaml-cpp, drogon, grpc_cpp_plugin
    fi

    print_header "Zion Server — Local Development Environment Setup"

    # Create temporary build directory
    mkdir -p "$BUILD_TEMP_DIR"

    # Pre-flight checks
    verify_prerequisites

    # Install APT packages
    if [ "$SKIP_APT" = false ]; then
        install_apt_packages
    else
        print_info "Skipping apt package installation"
    fi

    # Build from source
    if [ "$SKIP_BUILD_FROM_SRC" = false ]; then
        build_yaml_cpp
        build_drogon
        build_grpc_cpp_plugin
    else
        print_info "Skipping build-from-source dependencies"
    fi

    # Configure CMake
    configure_cmake

    # Setup environment
    setup_environment

    # Verify everything
    if verify_installation; then
        VERIFY_RESULT=true
    else
        VERIFY_RESULT=false
    fi

    # Cleanup
    cleanup_build_temp

    # Final summary
    print_header "Installation Complete!"

    if [ "$VERIFY_RESULT" = true ]; then
        echo -e "${GREEN}All dependencies installed successfully!${NC}"
        echo ""
        echo "Next steps:"
        echo "  1. Load environment variables:"
        echo "     ${BLUE}source ${PROJECT_ROOT}/.env.local${NC}"
        echo ""
        echo "  2. Start PostgreSQL (in separate terminal):"
        echo "     ${BLUE}docker compose -f docker/docker-compose.yml up postgres${NC}"
        echo ""
        echo "  3. Build the project:"
        echo "     ${BLUE}cd ${PROJECT_ROOT}${NC}"
        echo "     ${BLUE}cmake --preset debug${NC}"
        echo "     ${BLUE}cmake --build build/debug${NC}"
        echo ""
        echo "  4. Run tests:"
        echo "     ${BLUE}ctest --test-dir build/debug --output-on-failure${NC}"
        echo ""
    else
        echo -e "${YELLOW}Installation completed with warnings. Please review errors above.${NC}"
        exit 1
    fi
}

# Run main function
main "$@"
