#!/usr/bin/env bash
# scripts/generate_protos.sh — Generate C++ from proto files.
# Requires: protoc, grpc_cpp_plugin on PATH.
set -euo pipefail SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROTO_SRC="${REPO_ROOT}/../protos"
OUT_DIR="${REPO_ROOT}/build/debug/generated/proto" if [[ ! -d "${PROTO_SRC}" ]]; then echo "ERROR: Proto source directory not found: ${PROTO_SRC}" >&2 exit 1
fi mkdir -p "${OUT_DIR}" GRPC_PLUGIN="$(which grpc_cpp_plugin)" find "${PROTO_SRC}" -name '*.proto' | while read -r proto; do echo "Generating: ${proto}" protoc \ --proto_path="${PROTO_SRC}" \ --cpp_out="${OUT_DIR}" \ --grpc_out="${OUT_DIR}" \ --plugin=protoc-gen-grpc="${GRPC_PLUGIN}" \ "${proto}"
done echo "Proto generation complete → ${OUT_DIR}"
