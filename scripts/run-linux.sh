#!/bin/bash

# Simple run script for the physics engine
cd "$(dirname "$0")/build"

if [ ! -f "./OpenGLApp" ]; then
    echo "Error: Application not built yet!"
    echo "Please run ./build-linux.sh first"
    exit 1
fi

./OpenGLApp
