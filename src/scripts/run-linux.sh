#!/bin/bash

# Run script for the physics engine with error output
cd "$(dirname "$0")/../build"

if [ ! -f "./PhysicsEngine" ]; then
    echo "Error: Application not built yet!"
    echo "Please run scripts/build-linux.sh first"
    exit 1
fi

echo "Starting Custom Physics Engine..."
echo "If it crashes, check the error output below:"
echo ""

# Run with full error output
./PhysicsEngine 2>&1 || {
    EXIT_CODE=$?
    echo ""
    echo "=========================================="
    echo "Application exited with code: $EXIT_CODE"
    if [ $EXIT_CODE -eq 139 ]; then
        echo "Segmentation fault detected!"
        echo ""
        echo "To debug, run:"
        echo "  cd build && gdb ./PhysicsEngine"
        echo "  Then type: run"
        echo "  After crash, type: bt"
    fi
    echo "=========================================="
    exit $EXIT_CODE
}

