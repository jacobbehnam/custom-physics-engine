#!/bin/bash

# Exit on error
set -e

echo "=========================================="
echo "Custom Physics Engine - Linux Setup"
echo "=========================================="
echo ""

# Check if running on Linux
if [[ "$(uname)" != "Linux" ]]; then
    echo "Error: This script is for Linux only"
    exit 1
fi

# Detect package manager
if command -v apt-get &> /dev/null; then
    PKG_MANAGER="apt"
elif command -v dnf &> /dev/null; then
    PKG_MANAGER="dnf"
elif command -v pacman &> /dev/null; then
    PKG_MANAGER="pacman"
else
    echo "Error: Unsupported package manager. Please install dependencies manually."
    exit 1
fi

echo "Detected package manager: $PKG_MANAGER"
echo ""

# Check if dependencies are already installed
echo "Checking dependencies..."

MISSING_DEPS=()

# Check for essential tools
command -v cmake &> /dev/null || MISSING_DEPS+=("cmake")
command -v g++ &> /dev/null || MISSING_DEPS+=("g++")

# Check for libraries (basic check)
if [ "$PKG_MANAGER" = "apt" ]; then
    dpkg -l | grep -q libqt6widgets6 || MISSING_DEPS+=("Qt6")
    dpkg -l | grep -q libglm-dev || MISSING_DEPS+=("GLM")
    dpkg -l | grep -q libglfw3-dev || MISSING_DEPS+=("GLFW3")
fi

if [ ${#MISSING_DEPS[@]} -eq 0 ]; then
    echo "✓ All dependencies are already installed"
    echo ""
    echo "You can now run: ./build-linux.sh"
    exit 0
fi

echo "Missing dependencies: ${MISSING_DEPS[*]}"
echo ""
echo "The following SAFE packages will be installed from official repositories:"
echo ""

if [ "$PKG_MANAGER" = "apt" ]; then
    echo "  - build-essential (gcc, g++, make)"
    echo "  - cmake (build system)"
    echo "  - libgl1-mesa-dev (OpenGL development files)"
    echo "  - libglfw3-dev (GLFW windowing library)"
    echo "  - qt6-base-dev (Qt6 GUI framework)"
    echo "  - libqt6opengl6-dev (Qt6 OpenGL module)"
    echo "  - libqt6openglwidgets6 (Qt6 OpenGL widgets)"
    echo "  - libglm-dev (OpenGL Mathematics library)"
fi

echo ""
read -p "Install dependencies? (y/n): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Installation cancelled."
    exit 1
fi

echo ""
echo "Installing dependencies..."

if [ "$PKG_MANAGER" = "apt" ]; then
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        libgl1-mesa-dev \
        libglfw3-dev \
        qt6-base-dev \
        libqt6opengl6-dev \
        libqt6openglwidgets6 \
        libglm-dev
        
elif [ "$PKG_MANAGER" = "dnf" ]; then
    sudo dnf install -y \
        gcc-c++ \
        cmake \
        mesa-libGL-devel \
        glfw-devel \
        qt6-qtbase-devel \
        glm-devel
        
elif [ "$PKG_MANAGER" = "pacman" ]; then
    sudo pacman -S --needed \
        base-devel \
        cmake \
        mesa \
        glfw \
        qt6-base \
        glm
fi

echo ""
echo "=========================================="
echo "✓ Setup completed successfully!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Build: scripts/build-linux.sh"
echo "  2. Run: scripts/run-linux.sh"
echo ""
