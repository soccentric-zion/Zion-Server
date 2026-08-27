#!/bin/bash
# Check Docker prerequisites and environment setup

set -euo pipefail

echo "Checking Zion Docker prerequisites..."
echo ""

ERRORS=0
WARNINGS=0

# Check Docker
if command -v docker &>/dev/null; then
    DOCKER_VERSION=$(docker --version | awk '{print $3}' | sed 's/,//')
    echo "✓ Docker installed: $DOCKER_VERSION"
else
    echo "✗ Docker not found. Install from: https://docs.docker.com/get-docker/"
    ERRORS=$((ERRORS + 1))
fi

# Check Docker Compose
if command -v docker-compose &>/dev/null || docker compose version &>/dev/null; then
    if docker compose version &>/dev/null; then
        COMPOSE_VERSION=$(docker compose version | awk '{print $4}')
        echo "✓ Docker Compose (plugin) installed: $COMPOSE_VERSION"
    else
        COMPOSE_VERSION=$(docker-compose --version | awk '{print $4}')
        echo "✓ Docker Compose (standalone) installed: $COMPOSE_VERSION"
    fi
else
    echo "✗ Docker Compose not found."
    ERRORS=$((ERRORS + 1))
fi

# Check Docker buildx
if docker buildx version &>/dev/null; then
    BUILDX_VERSION=$(docker buildx version | awk '{print $2}')
    echo "✓ Docker buildx installed: $BUILDX_VERSION"
else
    echo "✗ Docker buildx not found. Install with:"
    echo "    Ubuntu 24.04: sudo apt install docker-buildx"
    echo "    Ubuntu 22.04: sudo apt install docker-buildx-plugin"
    echo "    Docker Desktop: Update to latest version"
    ERRORS=$((ERRORS + 1))
fi

# Check Docker daemon
if docker info &>/dev/null; then
    echo "✓ Docker daemon running"
else
    echo "✗ Docker daemon not running. Start with:"
    echo "    Linux: sudo systemctl start docker"
    echo "    macOS/Windows: Start Docker Desktop"
    ERRORS=$((ERRORS + 1))
fi

# Check .env file
if [ -f "../.env" ]; then
    echo "✓ .env file exists"
else
    echo "⚠ .env file not found. Create with:"
    echo "    cp .env.example .env"
    WARNINGS=$((WARNINGS + 1))
fi

# Check user permissions
if groups | grep -q docker; then
    echo "✓ User in docker group"
elif [ "$(id -u)" -eq 0 ]; then
    echo "⚠ Running as root (not recommended)"
    WARNINGS=$((WARNINGS + 1))
else
    echo "⚠ User not in docker group. Add with:"
    echo "    sudo usermod -aG docker $USER"
    echo "    (then log out and log back in)"
    WARNINGS=$((WARNINGS + 1))
fi

# Check disk space
AVAILABLE_GB=$(df -BG . | tail -1 | awk '{print $4}' | sed 's/G//')
if [ "$AVAILABLE_GB" -gt 10 ]; then
    echo "✓ Disk space: ${AVAILABLE_GB}GB available"
elif [ "$AVAILABLE_GB" -gt 5 ]; then
    echo "⚠ Disk space: ${AVAILABLE_GB}GB available (recommend 10GB+)"
    WARNINGS=$((WARNINGS + 1))
else
    echo "✗ Disk space: ${AVAILABLE_GB}GB available (need at least 5GB)"
    ERRORS=$((ERRORS + 1))
fi

# Check architecture
ARCH=$(uname -m)
case "$ARCH" in
    x86_64|aarch64|arm64)
        echo "✓ Architecture: $ARCH (supported)"
        ;;
    *)
        echo "⚠ Architecture: $ARCH (may not be supported)"
        WARNINGS=$((WARNINGS + 1))
        ;;
esac

echo ""
echo "─────────────────────────────────────────"

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo "✓ All checks passed! You're ready to build."
    echo ""
    echo "Next steps:"
    echo "  1. ./build.sh                     # Build Docker image"
    echo "  2. docker compose -f docker-compose.dev.yml up -d"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo "⚠ $WARNINGS warning(s) found. You can proceed but may encounter issues."
    exit 0
else
    echo "✗ $ERRORS error(s) found. Fix them before building."
    exit 1
fi
