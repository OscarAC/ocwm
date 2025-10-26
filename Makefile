# OCWM - Orbital Compositor & Window Manager
# Makefile for building the compositor

# Compiler and flags
CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -DWLR_USE_UNSTABLE -D_POSIX_C_SOURCE=200809L
LDFLAGS =

# Use pkg-config to find dependencies
PKGS = wayland-server wlroots pixman-1 xkbcommon glesv2 egl
LUA_PKG = $(shell pkg-config --exists lua5.4 && echo lua5.4 || echo lua)

CFLAGS += $(shell pkg-config --cflags $(PKGS) $(LUA_PKG)) -I$(PROTOCOLS_DIR)
LIBS = $(shell pkg-config --libs $(PKGS) $(LUA_PKG)) -lm

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
PROTOCOLS_DIR = protocols

# Target binary
TARGET = $(BIN_DIR)/ocwm

# Protocol generation
WAYLAND_PROTOCOLS = $(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER = $(shell pkg-config --variable=wayland_scanner wayland-scanner)

PROTOCOL_HEADERS = \
	$(PROTOCOLS_DIR)/xdg-shell-protocol.h

PROTOCOL_SOURCES = \
	$(PROTOCOLS_DIR)/xdg-shell-protocol.c

# Find all .c files
SRC_FILES = $(shell find $(SRC_DIR) -name '*.c')
SRC_OBJECTS = $(SRC_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
PROTOCOL_OBJECTS = $(PROTOCOL_SOURCES:.c=.o)
OBJECTS = $(SRC_OBJECTS) $(PROTOCOL_OBJECTS)
DEPS = $(SRC_OBJECTS:.o=.d)

# Build modes
DEBUG ?= 1

ifeq ($(DEBUG), 1)
    # Debug build
    CFLAGS += -g -O0 -DDEBUG
    BUILD_TYPE = debug
else
    # Release build - optimize for size
    CFLAGS += -O2 -DNDEBUG
    LDFLAGS += -s
    BUILD_TYPE = release
endif

# Protocol generation rules
$(PROTOCOLS_DIR)/xdg-shell-protocol.h:
	@mkdir -p $(PROTOCOLS_DIR)
	@echo "GEN $@"
	@$(WAYLAND_SCANNER) server-header $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

$(PROTOCOLS_DIR)/xdg-shell-protocol.c: $(PROTOCOLS_DIR)/xdg-shell-protocol.h
	@mkdir -p $(PROTOCOLS_DIR)
	@echo "GEN $@"
	@$(WAYLAND_SCANNER) private-code $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

$(PROTOCOLS_DIR)/%.o: $(PROTOCOLS_DIR)/%.c $(PROTOCOL_HEADERS)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Default target
all: $(PROTOCOL_HEADERS) $(TARGET)

# Create directories
$(BUILD_DIR) $(BIN_DIR):
	@mkdir -p $@

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Link the binary
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	@echo "LD $(TARGET)"
	@$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	@echo "Built $(TARGET) [$(BUILD_TYPE)]"
	@ls -lh $(TARGET) | awk '{print "Size: " $$5}'

# Release build (optimized for size)
release: clean
	@$(MAKE) DEBUG=0

# Tiny build (ultra-minimal, < 300KB)
tiny: clean
	@$(MAKE) DEBUG=0 CFLAGS="$(CFLAGS) -Os -flto" LDFLAGS="$(LDFLAGS) -flto -Wl,--gc-sections"

# Install
install: release
	@echo "Installing ocwm..."
	@install -Dm755 $(TARGET) /usr/local/bin/ocwm
	@install -Dm644 config/init.lua /etc/ocwm/init.lua
	@echo "Installed to /usr/local/bin/ocwm"

# Uninstall
uninstall:
	@rm -f /usr/local/bin/ocwm
	@rm -rf /etc/ocwm
	@echo "Uninstalled ocwm"

# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR) $(PROTOCOLS_DIR)
	@echo "Cleaned build artifacts"

# Run the compositor (basic, may need manual environment setup)
run: $(TARGET)
	@echo "Starting OCWM..."
	@$(TARGET)

# Run with startup script (handles environment automatically)
start: $(TARGET)
	@./start-ocwm.sh

# Show help
help:
	@echo "OCWM Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build debug version"
	@echo "  make release  - Build optimized release version"
	@echo "  make tiny     - Build ultra-minimal version (< 300KB)"
	@echo "  make install  - Install to system"
	@echo "  make clean    - Clean build artifacts"
	@echo "  make start    - Build and run with startup script (recommended)"
	@echo "  make run      - Build and run compositor directly"
	@echo "  make help     - Show this help"

# Include dependencies
-include $(DEPS)

.PHONY: all release tiny install uninstall clean run start help
