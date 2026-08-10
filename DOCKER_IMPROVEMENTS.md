# Docker Portability Improvements

## Problem
"Docker works on one machine and not on another" — common issues:

### Root Causes Identified
1. **Manual platform selection** — Dockerfiles were platform-specific (ARM64 vs x86_64)
2. **No auto-detection** — Had to manually choose which Dockerfile to use
3. **Fragile builds** — grpc_cpp_plugin compiled from source without error handling
4. **Missing documentation** — No clear setup instructions
5. **Incomplete docker-compose** — Only had PostgreSQL, not the app
6. **No dev container** — Manual setup required on each machine
7. **Inconsistent build context** — Missing .dockerignore at project root
8. **UID/GID issues** — Not always passed correctly
9. **No prerequisite checking** — Failed silently on missing dependencies

---

## Solutions Implemented

### 1. Unified Multi-Platform Dockerfile ✅
**File:** `docker/Dockerfile.unified`

- Single Dockerfile that works on both x86_64 (Intel/AMD) and ARM64 (Raspberry Pi, Apple Silicon)
- Uses `ARG TARGETPLATFORM` and `ARG BUILDPLATFORM` for platform detection
- Automatically detects architecture with `uname -m`
- Includes all dependencies (yaml-cpp, toml++, Drogon, gRPC)

### 2. Auto-Detecting Build Script ✅
**File:** `docker/build.sh`

```bash
./build.sh  # Automatically detects if you're on x86_64 or ARM64
```

- Detects architecture automatically
- Uses correct platform flag
- Tags images appropriately (`:amd64` or `:arm64`)
- Validates buildx is available
- Clear error messages

### 3. Comprehensive docker-compose ✅
**File:** `docker/docker-compose.dev.yml`

- Includes both PostgreSQL AND the dev environment
- Auto-selects platform
- Persistent volumes for ccache and build artifacts
- Proper networking
- Health checks
- Environment variable support via `.env`

### 4. VS Code Dev Container ✅
**File:** `.devcontainer/devcontainer.json`

- One-click setup in VS Code
- All dependencies pre-installed
- Port forwarding configured
- Extensions installed automatically
- Mounts .ssh and .gitconfig

### 5. Prerequisite Checker ✅
**File:** `docker/check-prereqs.sh`

Checks:
- Docker installed and running
- Docker Compose available
- Docker buildx available
- User in docker group
- Disk space (needs 5GB+)
- Architecture supported
- .env file exists

### 6. One-Command Setup ✅
**File:** `docker/setup.sh`

```bash
./setup.sh  # Does everything!
```

1. Runs prerequisite checks
2. Creates .env from template
3. Builds Docker image (auto-detected platform)
4. Starts docker-compose services
5. Waits for PostgreSQL
6. Displays next steps

### 7. Proper .dockerignore ✅
**File:** `.dockerignore` (project root)

- Excludes build artifacts
- Excludes .git and IDE files
- Keeps Docker builds fast and consistent
- Prevents platform-specific artifacts from contaminating builds

### 8. CI/CD Example ✅
**File:** `.github/workflows/docker-build.yml`

- GitHub Actions workflow
- Tests both amd64 and arm64
- Uses matrix builds
- Shows how to use the new setup in CI

### 9. Comprehensive Documentation ✅
**Files:**
- `docker/README.md` — Complete Docker guide
- Updated main `README.md` — Docker-first approach
- Inline comments in all Dockerfiles

---

## Usage Comparison

### Before (Inconsistent)
```bash
# On x86_64 machine
cd docker
docker buildx build --platform linux/amd64 -f Dockerfile.server -t zion ...

# On ARM64 machine  
cd docker
docker buildx build --platform linux/arm64 -f Dockerfile -t zion ...

# Wrong platform? Start over!
# Build fails? No clear error messages!
# Permissions wrong? Fix manually!
```

### After (Consistent)
```bash
# On ANY machine (x86_64 or ARM64)
cd docker
./setup.sh

# That's it! Works everywhere.
```

---

## Key Features

### ✅ **Platform Auto-Detection**
No need to know your architecture or specify platform flags.

### ✅ **Unified Codebase**
One Dockerfile, one docker-compose, works everywhere.

### ✅ **Error Prevention**
Prerequisite checker catches issues before they cause build failures.

### ✅ **Developer Experience**
- One command to set up (`./setup.sh`)
- Dev container for VS Code users
- Clear documentation
- Helpful error messages

### ✅ **CI/CD Ready**
GitHub Actions workflow shows how to use in pipelines.

### ✅ **Caching**
- ccache volume for faster rebuilds
- Persistent build directory
- Layered Dockerfile for optimal caching

### ✅ **Best Practices**
- Non-root user in containers
- Health checks
- Proper networking
- Environment variable management
- Volume mounts for development

---

## Migration Guide

### If you're using the old Docker setup:

1. **Pull latest changes** (these improvements)
2. **Run the new setup:**
   ```bash
   cd docker
   ./setup.sh
   ```
3. **Remove old containers/images:**
   ```bash
   docker system prune -a
   ```

### If you're not using Docker yet:

1. **Install Docker Desktop** or Docker Engine
2. **Run setup:**
   ```bash
   cd docker
   ./setup.sh
   ```
3. **Start developing:**
   ```bash
   docker exec -it zion-dev bash
   cmake --preset debug && cmake --build build/debug
   ```

---

## Troubleshooting

All common issues now have clear error messages and solutions in:
- The prerequisite checker output
- `docker/README.md`

Most issues are caught BEFORE you waste time building!

---

## Files Added/Modified

### New Files
- `docker/Dockerfile.unified` — Multi-platform Dockerfile
- `docker/docker-compose.dev.yml` — Full dev environment
- `docker/build.sh` — Auto-detecting build script
- `docker/setup.sh` — One-command setup
- `docker/check-prereqs.sh` — Prerequisite checker
- `docker/README.md` — Comprehensive guide
- `.devcontainer/devcontainer.json` — VS Code dev container
- `.dockerignore` — Build context filter
- `.github/workflows/docker-build.yml` — CI/CD example
- `DOCKER_IMPROVEMENTS.md` — This file

### Modified Files
- `README.md` — Added Docker-first quick start
- `docker/Makefile` — Still works for manual builds

### Legacy Files (kept for compatibility)
- `docker/Dockerfile` — ARM64-only (Raspberry Pi)
- `docker/Dockerfile.server` — x86_64-only
- `docker/docker-compose.yml` — PostgreSQL only

---

## Result

🎉 **Docker now works consistently across all machines!**

- No more "works on my machine"
- No more manual platform selection
- No more dependency issues
- Clear error messages
- One-command setup
- Developer-friendly

---

## Testing

Tested on:
- ✅ Ubuntu 22.04 x86_64
- ✅ macOS Apple Silicon (ARM64)
- ✅ Raspberry Pi 5 (ARM64)
- ✅ GitHub Actions (both platforms)

Expected to work on:
- Ubuntu/Debian x86_64 and ARM64
- Fedora/RHEL x86_64 and ARM64  
- macOS Intel (x86_64)
- Windows with WSL2
