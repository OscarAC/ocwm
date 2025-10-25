#!/bin/bash
#
# OCWM Dependency Installation Script
# Run this script to install all required dependencies

set -e

echo "╔════════════════════════════════════════════════════╗"
echo "║  OCWM Dependency Installation                      ║"
echo "╚════════════════════════════════════════════════════╝"
echo ""

# Detect package manager
if command -v xbps-install &> /dev/null; then
    PKG_MGR="xbps"
elif command -v pacman &> /dev/null; then
    PKG_MGR="pacman"
elif command -v apt &> /dev/null; then
    PKG_MGR="apt"
elif command -v dnf &> /dev/null; then
    PKG_MGR="dnf"
else
    echo "Error: No supported package manager found"
    exit 1
fi

echo "Detected package manager: $PKG_MGR"
echo ""

# Install based on package manager
case $PKG_MGR in
    xbps)
        echo "Installing dependencies for Void Linux..."
        sudo xbps-install -Sy \
            wayland-devel \
            wayland-protocols \
            wlroots-devel \
            pixman-devel \
            libxkbcommon-devel \
            lua54-devel \
            mesa-devel \
            eudev-libudev-devel \
            libinput-devel \
            libdrm-devel
        ;;
    pacman)
        echo "Installing dependencies for Arch Linux..."
        sudo pacman -S --needed \
            wayland \
            wayland-protocols \
            wlroots \
            pixman \
            libxkbcommon \
            lua \
            mesa \
            libinput
        ;;
    apt)
        echo "Installing dependencies for Debian/Ubuntu..."
        sudo apt update
        sudo apt install -y \
            libwayland-dev \
            wayland-protocols \
            libwlroots-dev \
            libpixman-1-dev \
            libxkbcommon-dev \
            liblua5.4-dev \
            libgles2-mesa-dev \
            libegl1-mesa-dev \
            libinput-dev \
            libdrm-dev
        ;;
    dnf)
        echo "Installing dependencies for Fedora..."
        sudo dnf install -y \
            wayland-devel \
            wayland-protocols-devel \
            wlroots-devel \
            pixman-devel \
            libxkbcommon-devel \
            lua-devel \
            mesa-libEGL-devel \
            mesa-libGLES-devel \
            libinput-devel \
            libdrm-devel
        ;;
esac

echo ""
echo "✓ Dependencies installed successfully!"
echo ""
echo "You can now build OCWM with: make"
