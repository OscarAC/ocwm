#!/bin/bash
# OCWM Startup Script
# Handles environment setup and backend selection

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OCWM_BIN="${SCRIPT_DIR}/bin/ocwm"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if binary exists
if [ ! -f "$OCWM_BIN" ]; then
    print_error "OCWM binary not found at $OCWM_BIN"
    print_info "Run 'make' to build the compositor"
    exit 1
fi

# Setup XDG_RUNTIME_DIR if not set
if [ -z "$XDG_RUNTIME_DIR" ]; then
    export XDG_RUNTIME_DIR="/tmp/ocwm-runtime-${UID}"
    mkdir -p "$XDG_RUNTIME_DIR"
    chmod 700 "$XDG_RUNTIME_DIR"
    print_info "Created XDG_RUNTIME_DIR: $XDG_RUNTIME_DIR"
fi

# Determine backend mode
BACKEND=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --headless)
            BACKEND="headless"
            shift
            ;;
        --x11)
            BACKEND="x11"
            shift
            ;;
        --wayland)
            BACKEND="wayland"
            shift
            ;;
        --drm|--tty)
            BACKEND=""  # Let wlroots auto-detect DRM
            shift
            ;;
        --help|-h)
            echo "OCWM Startup Script"
            echo ""
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --headless    Run in headless mode (no display)"
            echo "  --x11         Run nested in X11 session"
            echo "  --wayland     Run nested in Wayland session"
            echo "  --drm, --tty  Run directly on DRM/TTY (default if no session detected)"
            echo "  --help, -h    Show this help message"
            echo ""
            echo "Environment:"
            echo "  XDG_RUNTIME_DIR  Runtime directory for Wayland socket"
            echo "                   (auto-created if not set)"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Auto-detect backend if not specified
if [ -z "$BACKEND" ]; then
    if [ -n "$WAYLAND_DISPLAY" ]; then
        BACKEND="wayland"
        print_info "Auto-detected Wayland session, running nested"
    elif [ -n "$DISPLAY" ]; then
        BACKEND="x11"
        print_info "Auto-detected X11 session, running nested"
    else
        print_info "No graphical session detected, attempting DRM backend"
        print_warn "DRM backend requires running from a TTY with proper seat permissions"
        # Don't set WLR_BACKENDS, let wlroots auto-detect
    fi
fi

# Export backend if specified
if [ -n "$BACKEND" ]; then
    export WLR_BACKENDS="$BACKEND"
    print_info "Using backend: $BACKEND"
fi

# Set renderer if needed
if [ "$BACKEND" = "headless" ]; then
    print_info "Headless mode: Output will not be visible"
fi

# Start OCWM
print_info "Starting OCWM..."
print_info "Binary: $OCWM_BIN"
print_info "Runtime dir: $XDG_RUNTIME_DIR"

exec "$OCWM_BIN"
