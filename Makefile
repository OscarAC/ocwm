# OCWM - Orbital Compositor & Window Manager
# Makefile for building the compositor

# Compiler and flags
CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic
LDFLAGS =

# Use pkg-config to find dependencies
PKGS = wayland-server wlroots pixman-1 xkbcommon glesv2 egl
LUA_PKG = $(shell pkg-config --exists lua5.4 && echo lua5.4 || echo lua)

CFLAGS += $(shell pkg-config --cflags $(PKGS) $(LUA_PKG))
LIBS = $(shell pkg-config --libs $(PKGS) $(LUA_PKG)) -lm

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Target binary
TARGET = $(BIN_DIR)/ocwm

# Find all .c files
SOURCES = $(shell find $(SRC_DIR) -name '*.c')
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

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

# Default target
all: $(TARGET)

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
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Run the compositor
run: $(TARGET)
	@echo "Starting OCWM..."
	@$(TARGET)

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
	@echo "  make run      - Build and run compositor"
	@echo "  make help     - Show this help"

# Include dependencies
-include $(DEPS)

.PHONY: all release tiny install uninstall clean run help
