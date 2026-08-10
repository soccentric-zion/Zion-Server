# Zynq UltraScale+ (Zynq UltraScale+) build container
# Uses Ubuntu 22.04 LTS for ARM64 — runs natively on Zynq UltraScale+ or via QEMU on x86_64
FROM ubuntu:22.04 ARG UID=1000
ARG GID=1000 # Install build dependencies
RUN apt-get update && apt-get install -y \ build-essential \ cmake \ git \ wget \ protobuf-compiler \ libprotobuf-dev \ libprotoc-dev \ libgrpc++-dev \ libgrpc-dev \ libsqlite3-dev \ libspdlog-dev \ libgtest-dev \ binutils \ pkg-config \ ninja-build \ vim \ htop \ libboost-system-dev \ libboost-date-time-dev \ && rm -rf/var/lib/apt/lists/* # toml++ is not packaged for Ubuntu 22.04 ARM64 — install header-only lib from source
RUN git clone --depth=1 --branch v3.4.0 https://github.com/marzer/tomlplusplus.git/tmp/tomlplusplus \ && cmake -S/tmp/tomlplusplus -B/tmp/tomlplusplus/build \ -DCMAKE_INSTALL_PREFIX=/usr \ -DCMAKE_INSTALL_LIBDIR=lib \ && cmake --install/tmp/tomlplusplus/build \ && rm -rf/tmp/tomlplusplus # yaml-cpp 0.7.x (Ubuntu 22.04) lacks the yaml-cpp::yaml-cpp imported target; build 0.8.0
RUN git clone --depth=1 --branch 0.8.0 https://github.com/jbeder/yaml-cpp.git/tmp/yaml-cpp \ && cmake -S/tmp/yaml-cpp -B/tmp/yaml-cpp/build \ -DCMAKE_INSTALL_PREFIX=/usr \ -DCMAKE_INSTALL_LIBDIR=lib \ -DYAML_CPP_BUILD_TESTS=OFF \ && cmake --build/tmp/yaml-cpp/build --parallel \ && cmake --install/tmp/yaml-cpp/build \ && rm -rf/tmp/yaml-cpp # Build grpc_cpp_plugin from source files.
# The plugin only needs libprotoc (already installed above).
# We download ALL headers from src/compiler/ (protobuf_plugin.h transitively
# includes every language generator helper at v1.30.2).
RUN apt-get update && apt-get install -y wget && \ rm -rf/var/lib/apt/lists/* && \ mkdir -p/tmp/grpc_plugin_root/src/compiler && \ cd/tmp/grpc_plugin_root/src/compiler && \ BASE="https://raw.githubusercontent.com/grpc/grpc/v1.30.2/src/compiler" && \ for f in \ config.h config_protobuf.h generator_helpers.h schema_interface.h \ protobuf_plugin.h \ cpp_generator.h cpp_generator.cc cpp_generator_helpers.h \ cpp_plugin.h cpp_plugin.cc \ csharp_generator.h csharp_generator_helpers.h \ node_generator.h node_generator_helpers.h \ objective_c_generator.h objective_c_generator_helpers.h \ php_generator.h php_generator_helpers.h \ python_generator.h python_generator_helpers.h python_private_generator.h \ ruby_generator.h ruby_generator_helpers-inl.h \ ruby_generator_map-inl.h ruby_generator_string-inl.h; do \ wget -q "${BASE}/${f}" || { echo "FAIL: ${f}"; exit 1; }; \ done && \ cd/tmp/grpc_plugin_root && \ g++ -std=c++14 -O2 -o/usr/local/bin/grpc_cpp_plugin \ src/compiler/cpp_plugin.cc \ src/compiler/cpp_generator.cc \ -I. -lprotoc -lprotobuf && \ rm -rf/tmp/grpc_plugin_root && \ echo "[Dockerfile] grpc_cpp_plugin installed at $(which grpc_cpp_plugin)" # Install additional dependencies
RUN apt-get update && apt-get install -y build-essential cmake git sudo ccache file \ && rm -rf/var/lib/apt/lists/* # --no-log-init
# Workaround for "Docker build hangs when adding user with a large UID"
# https://stackoverflow.com/questions/73351423
RUN groupadd -g ${GID:-1000} rpi && \ useradd -m --no-log-init --uid ${UID:-1000} --gid ${GID:-1000} rpi && \ usermod -aG sudo rpi USER rpi # Enable ccache: prepend ccache symlinks to PATH so gcc/g++/cc/c++ go through ccache
ENV PATH="/usr/lib/ccache:${PATH}" # Create ccache directory so Docker volume mount inherits correct ownership
RUN mkdir -p/home/rpi/.ccache # Set working directory for build
WORKDIR/workspace
