#!/bin/bash
# One-command setup for Zion development environment

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "═══════════════════════════════════════════════════════════"
echo "  Zion Server — Docker Development Setup"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Step 1: Check prerequisites
echo "[1/5] Checking prerequisites..."
if ! "$SCRIPT_DIR/check-prereqs.sh"; then
    echo ""
    echo "Fix the errors above and try again."
    exit 1
fi
echo ""

# Step 2: Create .env if needed
echo "[2/5] Setting up environment..."
if [ ! -f "$PROJECT_ROOT/.env" ]; then
    echo "Creating .env from .env.example..."
    cp "$PROJECT_ROOT/.env.example" "$PROJECT_ROOT/.env"
    echo "⚠ Please edit .env and set ZION_SESSION_SECRET and database password!"
else
    echo ".env already exists"
fi
echo ""

# Step 3: Build Docker image
echo "[3/5] Building Docker image..."
cd "$SCRIPT_DIR"
if ! ./build.sh; then
    echo "Build failed!"
    exit 1
fi
echo ""

# Step 4: Start services
echo "[4/5] Starting Docker Compose services..."
cd "$SCRIPT_DIR"
docker compose -f docker-compose.dev.yml up -d
echo ""

# Step 5: Wait for database
echo "[5/5] Waiting for PostgreSQL to be ready..."
for i in {1..30}; do
    if docker compose -f docker-compose.dev.yml exec -T postgres pg_isready -U zion &>/dev/null; then
        echo "PostgreSQL is ready!"
        break
    fi
    if [ $i -eq 30 ]; then
        echo "PostgreSQL failed to start in time"
        docker compose -f docker-compose.dev.yml logs postgres
        exit 1
    fi
    echo -n "."
    sleep 1
done
echo ""

echo "═══════════════════════════════════════════════════════════"
echo "✓ Setup complete!"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Next steps:"
echo "  1. Enter dev container:"
echo "     docker exec -it zion-dev bash"
echo ""
echo "  2. Build the project:"
echo "     cmake --preset debug"
echo "     cmake --build build/debug"
echo ""
echo "  3. Run tests:"
echo "     ctest --test-dir build/debug --output-on-failure"
echo ""
echo "  4. Run migrations:"
echo "     psql -h postgres -U zion -d zion -f migrations/0001_init.sql"
echo ""
echo "  5. Start server:"
echo "     ./build/debug/zion-server"
echo ""
echo "Services:"
echo "  - PostgreSQL: localhost:5432"
echo "  - gRPC:       localhost:50051"
echo "  - HTTP API:   localhost:8080"
echo "  - Metrics:    localhost:9100"
echo ""
echo "Manage services:"
echo "  View logs:  docker compose -f docker/docker-compose.dev.yml logs -f"
echo "  Stop:       docker compose -f docker/docker-compose.dev.yml down"
echo "  Restart:    docker compose -f docker/docker-compose.dev.yml restart"
