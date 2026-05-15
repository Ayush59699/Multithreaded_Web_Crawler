#!/bin/bash
# Quick build script for the web crawler

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════╗"
echo "║    Multithreaded Web Crawler - Build Script           ║"
echo "╚════════════════════════════════════════════════════════╝"
echo

# Check for required tools
echo "[1/5] Checking prerequisites..."

if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Installing..."
    sudo apt-get install -y cmake
fi

if ! command -v gcc &> /dev/null; then
    echo "❌ GCC not found. Installing..."
    sudo apt-get install -y build-essential
fi

if ! pkg-config --exists libcurl; then
    echo "❌ libcurl not found. Installing..."
    sudo apt-get install -y libcurl4-openssl-dev
fi

echo "✓ Prerequisites OK"
echo

# Create build directory
echo "[2/5] Creating build directory..."
if [ -d "build" ]; then
    echo "  Removing existing build directory..."
    rm -rf build
fi
mkdir build
cd build
echo "✓ Build directory created"
echo

# Run CMake
echo "[3/5] Running CMake..."
cmake ..
echo "✓ CMake configuration done"
echo

# Build
echo "[4/5] Building..."
make -j$(nproc)
echo "✓ Build complete"
echo

# Verify
echo "[5/5] Verifying build..."
if [ -f "crawler" ]; then
    echo "✓ Executable created: ./crawler"
    echo
    ./crawler 2>&1 | head -10
else
    echo "❌ Build failed - executable not found"
    exit 1
fi

echo
echo "╔════════════════════════════════════════════════════════╗"
echo "║         BUILD SUCCESSFUL - Ready to crawl!            ║"
echo "╚════════════════════════════════════════════════════════╝"
echo
echo "Run with:"
echo "  ./crawler <seed_url> <max_pages> <num_threads>"
echo
echo "Example:"
echo "  ./crawler https://example.com 50 4"
echo
