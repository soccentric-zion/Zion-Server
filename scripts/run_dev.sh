#!/usr/bin/env bash
# scripts/run_dev.sh — Build (debug preset) and launch server with .env loaded.
set -euo pipefail SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)" ENV_FILE="${REPO_ROOT}/.env"
if [[ -f "${ENV_FILE}" ]]; then set -o allexport source "${ENV_FILE}" set +o allexport
fi cmake --preset debug -S "${REPO_ROOT}" -B "${REPO_ROOT}/build/debug"
cmake --build "${REPO_ROOT}/build/debug" --target zion-server -j"$(nproc)" exec "${REPO_ROOT}/build/debug/zion-server"
