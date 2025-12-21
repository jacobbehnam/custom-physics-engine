#!/bin/bash

# Exit on error
set -e

echo "=========================================="
echo "Building Custom Physics Engine"
echo "=========================================="
echo ""

# Check dependencies
if ! command -v cmake &> /dev/null || ! command -v g++ &> /dev/null; then
    echo "Error: Dependencies not installed!"
    echo "Please run ./setup-linux.sh first"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo ""
echo "Compiling..."
cmake --build . -j$(nproc)

echo ""
echo "=========================================="
echo "✓ Build completed successfully!"
echo "=========================================="
echo ""
echo "To run: ./run-linux.sh"
echo ""
