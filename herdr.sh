#!/usr/bin/env bash
# herdr.sh — open this repo's Herdr session (https://herdr.dev).
#
# Managed file: a byte-identical copy lives in every repo of the fleet — the
# root orchestrator, each of the 9 projects, and each leaf. It is deliberately
# self-contained: it sources nothing, so a leaf cloned on its own still works.
# Regenerate every copy with project-managers/script/sync_herdr.sh.
#
# The tier is detected at runtime from the directory layout:
#
#   root repo    (has pm/projects.txt)  session "project_manager"
#                                       + one workspace per project in Projects/
#   project repo (has Project/)         session "<repo-name lowercased>"
#                                       + one workspace per leaf in Project/
#   leaf repo    (neither)              session "<repo-name lowercased>"
#                                       no child workspaces
#
# The first workspace is always this repo itself; children follow in name
# order. If the session already exists the script skips creation entirely and
# just attaches. Each workspace gets a single shell at its repo root — no
# extra tabs, panes, processes or agents.

set -Eeuo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_NAME="$(basename "$REPO_DIR")"

die() { printf 'herdr.sh: %s\n' "$*" >&2; exit 1; }
note() { printf 'herdr.sh: %s\n' "$*"; }

lower() { printf '%s' "$1" | tr '[:upper:]' '[:lower:]'; }

# ── Preconditions ─────────────────────────────────────────────────────────────
# Herdr clients cannot nest: attaching from inside a managed pane would try to
# run a session within a session.
if [[ "${HERDR_ENV:-}" == "1" ]]; then
    die "already running inside a Herdr pane — detach first (or use \`herdr session attach\` from outside)."
fi

command -v herdr >/dev/null 2>&1 ||
    die "herdr not found on PATH — install it with: curl -fsSL https://herdr.dev/install.sh | sh"

# ── Tier detection ────────────────────────────────────────────────────────────
if [[ -f "$REPO_DIR/pm/projects.txt" ]]; then
    SESSION="project_manager"          # root is a named special case
    CHILD_DIR="$REPO_DIR/Projects"
elif [[ -d "$REPO_DIR/Project" ]]; then
    SESSION="$(lower "$REPO_NAME")"
    CHILD_DIR="$REPO_DIR/Project"
else
    SESSION="$(lower "$REPO_NAME")"
    CHILD_DIR=""
fi

# ── Already there? Attach and stop. ───────────────────────────────────────────
session_exists() {
    # `session list --json` emits compact JSON; splitting on commas isolates
    # each "name":"..." field so no JSON parser is needed.
    herdr session list --json 2>/dev/null |
        tr ',' '\n' |
        grep -qx "\"name\":\"$SESSION\""
}

if session_exists; then
    note "session '$SESSION' exists — attaching."
    cd "$REPO_DIR"
    exec herdr session attach "$SESSION"
fi

# ── Plan the workspaces ───────────────────────────────────────────────────────
# This repo always comes first, so the session opens on the parent rather than
# on whichever child happens to sort earliest. Children follow in name order;
# a leaf has none.
WORKSPACES=("$REPO_DIR")
if [[ -n "$CHILD_DIR" && -d "$CHILD_DIR" ]]; then
    for child in "$CHILD_DIR"/*/; do
        [[ -d "$child" ]] || continue
        WORKSPACES+=("${child%/}")
    done
fi

# ── Bootstrap: headless server, then one workspace each ───────────────────────
# The workspaces must exist before a client attaches, so bring the session up
# headless first and talk to it over its socket.
plural=s; [[ ${#WORKSPACES[@]} -eq 1 ]] && plural=""
note "creating session '$SESSION' with ${#WORKSPACES[@]} workspace${plural}."
cd "$REPO_DIR"
nohup herdr --session "$SESSION" server >/dev/null 2>&1 &

ready=0
for _ in $(seq 1 60); do
    if herdr --session "$SESSION" workspace list >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 0.25
done
[[ $ready -eq 1 ]] || die "timed out waiting for the '$SESSION' server to start."

for workspace in "${WORKSPACES[@]}"; do
    label="$(basename "$workspace")"
    # --no-focus keeps creation order from stealing focus, so this repo's own
    # workspace stays selected and attaching lands there.
    if ! herdr --session "$SESSION" workspace create \
        --cwd "$workspace" --label "$label" --no-focus >/dev/null 2>&1; then
        note "warning: could not create workspace '$label'."
    fi
done

exec herdr session attach "$SESSION"
