#!/bin/bash
# OCWM Debug Startup Script - Logs output to file
# Use this to debug issues with keybindings and see what's happening

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OCWM_BIN="${SCRIPT_DIR}/bin/ocwm"
LOG_FILE="${SCRIPT_DIR}/ocwm.log"

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
            echo "OCWM Debug Startup Script"
            echo ""
            echo "This script runs OCWM and saves logs to: $LOG_FILE"
            echo ""
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --headless    Run in headless mode (no display)"
            echo "  --x11         Run nested in X11 session"
            echo "  --wayland     Run nested in Wayland session"
            echo "  --drm, --tty  Run directly on DRM/TTY"
            echo "  --help, -h    Show this help message"
            echo ""
            echo "Emergency Exit: Alt+Escape (hardcoded)"
            echo ""
            echo "After running, check logs with:"
            echo "  tail -f $LOG_FILE"
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
    fi
fi

# Export backend if specified
if [ -n "$BACKEND" ]; then
    export WLR_BACKENDS="$BACKEND"
    print_info "Using backend: $BACKEND"
fi

# Set higher log verbosity
export WLR_DEBUG=1

# Start OCWM with logging
print_info "Starting OCWM with debug logging..."
print_info "Binary: $OCWM_BIN"
print_info "Runtime dir: $XDG_RUNTIME_DIR"
print_info "Log file: $LOG_FILE"
print_info ""
print_warn "Emergency exit: Press Alt+Escape"
print_info ""
print_info "To view logs in another terminal:"
print_info "  tail -f $LOG_FILE"
print_info ""

# Run OCWM and redirect output to log file
exec "$OCWM_BIN" 2>&1 | tee "$LOG_FILE"
