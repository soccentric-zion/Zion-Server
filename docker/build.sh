#!/bin/bash
# Portable Docker build script — auto-detects platform and builds the right image set -euo pipefail # Detect architecture
ARCH=$(uname -m)
case "$ARCH" in x86_64) PLATFORM="linux/amd64" TAG_SUFFIX="amd64" ;; aarch64|arm64) PLATFORM="linux/arm64" TAG_SUFFIX="arm64" ;; *) echo "ERROR: Unsupported architecture: $ARCH" echo "Supported: x86_64, aarch64/arm64" exit 1 ;;
esac IMAGE_NAME="${IMAGE_NAME:-zion}"
DOCKERFILE="${DOCKERFILE:-Dockerfile.unified}" echo "─────────────────────────────────────────────────────"
echo "Building Zion Docker image"
echo " Architecture: $ARCH"
echo " Platform: $PLATFORM"
echo " Image: $IMAGE_NAME:$TAG_SUFFIX"
echo " Dockerfile: $DOCKERFILE"
echo "─────────────────────────────────────────────────────" # Check if buildx is available
if docker buildx version &>/dev/null; then echo "Using docker buildx..." # Build with buildx docker buildx build \ --platform "$PLATFORM" \ --load \ --build-arg UID="$(id -u)" \ --build-arg GID="$(id -g)" \ -f "$DOCKERFILE" \ -t "$IMAGE_NAME:$TAG_SUFFIX" \ -t "$IMAGE_NAME:latest" \ ..
else echo "⚠ docker buildx not found, falling back to regular docker build" echo " (To use buildx: sudo apt install docker-buildx-plugin)" echo "" # Fallback to regular docker build docker build \ --build-arg UID="$(id -u)" \ --build-arg GID="$(id -g)" \ -f "$DOCKERFILE" \ -t "$IMAGE_NAME:$TAG_SUFFIX" \ -t "$IMAGE_NAME:latest" \ ..
fi echo "✓ Build complete: $IMAGE_NAME:$TAG_SUFFIX"
echo ""
echo "Run with:"
echo " docker run -it --rm $IMAGE_NAME:latest"
echo "Or use docker-compose.dev.yml for full dev environment"
