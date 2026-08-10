# Docker Setup for Zion Server This directory contains Docker configurations for building and running Zion Server across different platforms. ## Quick Start ### Option 1: VS Code Dev Container (Recommended)
The easiest way to get a consistent development environment: 1. Install VS Code with the "Dev Containers" extension
2. Open this project in VS Code
3. Press `F1` → "Dev Containers: Reopen in Container"
4. Wait for the container to build and start Everything (database, build tools, dependencies) will be configured automatically. ### Option 2: Docker Compose (Manual)
For a full development environment with database: ```bash
# Copy environment file
cp ../.env.example ../.env
# Edit .env with your settings # Start all services
docker compose -f docker-compose.dev.yml up -d # Attach to the dev container
docker exec -it zion-dev bash # Inside the container, build the project:
cmake --preset debug
cmake --build build/debug
``` ### Option 3: Standalone Build Script
For just building the Docker image: ```bash
cd docker
chmod +x build.sh
./build.sh
``` The script auto-detects your platform (x86_64 or ARM64) and builds accordingly. ## Files Overview | File | Purpose |
|------|---------|
| `Dockerfile.unified` | Multi-platform Dockerfile (x86_64 + ARM64) |
| `docker-compose.dev.yml` | Full dev stack (app + PostgreSQL) |
| `docker-compose.yml` | PostgreSQL only (legacy) |
| `build.sh` | Auto-detecting build script |
| `Dockerfile` | ARM64-specific (Zynq UltraScale+) |
| `Dockerfile.server` | x86_64-specific (legacy) | ## Platform Support The **unified** approach (`Dockerfile.unified` + `build.sh`) automatically works on: - **x86_64** (Intel/AMD) — most desktops, servers, CI
- **ARM64** (aarch64) — Zynq UltraScale+, Apple Silicon, AWS Graviton No manual platform selection needed! ## Architecture Auto-Detection The build script detects your architecture: ```bash
$ ./build.sh
Building Zion Docker image Architecture: x86_64 Platform: linux/amd64 Image: zion-dev:amd64
``` ## Troubleshooting ### "docker buildx not found"
Install buildx:
- **Docker Desktop**: Built-in, just update Docker
- **Linux**: `sudo apt install docker-buildx-plugin` ### Build fails with "grpc_cpp_plugin not found"
The Dockerfile builds it from source. If it fails:
1. Check internet connection (needs to clone from GitHub)
2. Ensure you have enough disk space
3. Check Docker logs: `docker logs <container-id>` ### ccache not working
Ensure the volume is mounted:
```bash
docker volume ls | grep ccache
docker volume inspect zion_ccache
``` ### Permission issues with mounted volumes
The container user UID/GID should match your host user:
```bash
# Check your UID/GID
id # Rebuild with your UID/GID
UID=$(id -u) GID=$(id -g) docker compose -f docker-compose.dev.yml build
``` ### Database connection refused
Ensure PostgreSQL is healthy:
```bash
docker compose -f docker-compose.dev.yml ps
docker compose -f docker-compose.dev.yml logs postgres
``` ## Environment Variables See `../.env.example` for all configuration options. Key variables: - `ZION_DB_HOST` — Database host (use `postgres` in Docker Compose)
- `ZION_GRPC_LISTEN` — gRPC server address
- `ZION_HTTP_LISTEN` — HTTP API address ## Development Workflow ```bash
# 1. Start services
docker compose -f docker-compose.dev.yml up -d # 2. Enter dev container
docker exec -it zion-dev bash # 3. Build
cmake --preset debug && cmake --build build/debug # 4. Run tests
ctest --test-dir build/debug --output-on-failure # 5. Run migrations
psql -h postgres -U zion -d zion -f/workspace/migrations/0001_init.sql # 6. Start server
./build/debug/zion-server # 7. View logs
docker compose -f docker-compose.dev.yml logs -f zion-dev # 8. Stop all
docker compose -f docker-compose.dev.yml down
``` ## Continuous Integration For CI/CD pipelines, use the unified Dockerfile with explicit platform: ```yaml
# GitHub Actions example
- name: Build Docker image run: | docker buildx build \ --platform linux/amd64 \ --build-arg UID=1000 \ --build-arg GID=1000 \ -f docker/Dockerfile.unified \ -t zion-server:${{ github.sha }} \ .
``` ## Legacy Files - `Dockerfile` — Old ARM64-only build (kept for reference)
- `Dockerfile.server` — Old x86_64-only build (kept for reference)
- `docker-compose.yml` — PostgreSQL only (use docker-compose.dev.yml instead) These are maintained for backwards compatibility but **Dockerfile.unified is recommended** for new setups.
