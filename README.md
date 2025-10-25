# OCWM - Window Manager

A lightweight, scriptable Wayland compositor combining the power of tiling window management with smooth animations and visual effects.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Language: C](https://img.shields.io/badge/Language-C-00599C.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Wayland](https://img.shields.io/badge/Protocol-Wayland-orange.svg)](https://wayland.freedesktop.org/)

---

## About

OCWM is a modern Wayland compositor built from the ground up with **wlroots**, designed for users who want the efficiency of tiling window management with the flexibility of complete Lua scripting and the polish of smooth animations.

Unlike traditional compositors that require recompilation for configuration changes, OCWM embraces runtime flexibility through a comprehensive Lua API, allowing you to customize every aspect of your window management experience through simple configuration files.

### Why OCWM?

- **Fully Scriptable**: Complete Lua API for keybindings, layouts, events, and window rules
- **Tiling + Floating**: Four built-in layouts (master-stack, grid, monocle, floating)
- **Beautiful Effects**: 60 FPS animations with professional easing functions
- **Minimal Footprint**: Sub-700KB binary, under 50MB memory usage
- **Hot-Reload**: Change configurations without restarting your compositor
- **Modern Architecture**: Built on wlroots, the same foundation as Sway and Hyprland

---

## Features

### Core Functionality
- **Wayland Native**: Pure Wayland compositor using wlroots 0.17+
- **Multi-Monitor Support**: Full support for multiple displays
- **Window Management**: Floating and tiling window management with smooth transitions
- **Workspaces**: 9 virtual desktops for organizing your workflow
- **Input Handling**: Complete keyboard and mouse support via libinput and XKB

### Tiling Layouts
- **Floating**: Traditional desktop-style window placement
- **Master-Stack**: dwm-inspired layout with configurable master area
- **Grid**: Automatic grid arrangement for multitasking
- **Monocle**: Fullscreen focus mode

### Visual Effects
- **Smooth Animations**: Window open/close animations at 60 FPS
- **Easing Functions**: 5 professional easing curves (linear, ease-in-out, ease-out, elastic, bounce)
- **Transparency**: Per-window opacity control
- **Blur Support**: Framework ready for GPU-accelerated blur effects

### Lua Scripting
- **60+ API Functions**: Comprehensive control over window management
- **Custom Keybindings**: Define any keybinding with modifier support
- **Event Hooks**: React to window open/close/focus events
- **Window Rules**: Apply effects and behaviors based on application ID
- **Hot-Reload**: Configuration changes take effect immediately

---

## Installation

### Dependencies

OCWM requires the following libraries:

- wayland-server
- wlroots (>= 0.17)
- pixman
- libxkbcommon
- lua5.4 (or lua)
- mesa (EGL + GLESv2)
- libinput
- libdrm

### Distribution-Specific Instructions

#### Arch Linux
```bash
sudo pacman -S wayland wayland-protocols wlroots pixman libxkbcommon lua
```

#### Ubuntu/Debian
```bash
sudo apt install libwayland-dev wayland-protocols wlroots libpixman-1-dev \
                 libxkbcommon-dev liblua5.4-dev libegl-dev libgles2-mesa-dev
```

#### Fedora
```bash
sudo dnf install wayland-devel wayland-protocols-devel wlroots-devel \
                 pixman-devel libxkbcommon-devel lua-devel mesa-libEGL-devel
```

#### Void Linux (or use automated script)
```bash
./setup-deps.sh  # Detects your distribution and installs dependencies
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/ocwm.git
cd ocwm

# Install dependencies (automated)
./setup-deps.sh

# Build (debug version with symbols)
make

# Or build optimized release version
make release

# Install to /usr/local/bin (optional)
sudo make install
```

#### Build Targets
- `make` - Debug build with symbols (~900KB)
- `make release` - Optimized build (~650KB)
- `make tiny` - Ultra-minimal build (~350KB)
- `make clean` - Clean build artifacts

---

## Quick Start

### 1. Configuration

Copy the example configuration to your config directory:

```bash
mkdir -p ~/.config/ocwm
cp config/init.lua ~/.config/ocwm/init.lua
```

Edit `~/.config/ocwm/init.lua` to customize keybindings, layouts, and effects.

### 2. Running OCWM

From a TTY (Ctrl+Alt+F2):

```bash
./bin/ocwm
```

OCWM will start and display the Wayland socket (e.g., `wayland-1`).

### 3. Launching Applications

From another TTY or SSH session:

```bash
export WAYLAND_DISPLAY=wayland-1
alacritty &
firefox &
```

Or use the built-in keybindings:
- **Super+Return** - Launch terminal
- **Super+d** - Application launcher (wofi)
- **Super+w** - Launch browser

---

## Configuration

OCWM is configured entirely through Lua. Here's a simple example:

```lua
-- Enable effects with 250ms animations
ocwm.effects.enable(true)
ocwm.effects.set_duration(250, 200)

-- Keybinding: Super+Return launches terminal
ocwm.bind("Mod+Return", function()
    ocwm.spawn("alacritty")
end)

-- Event hook: Make terminals slightly transparent
ocwm.on("window_open", function(window)
    local app = ocwm.window.get_app_id(window)
    if app == "alacritty" then
        ocwm.window.set_opacity(window, 0.95)
    end
end)

-- Workspace switching: Super+1 through Super+9
for i = 1, 9 do
    ocwm.bind("Mod+" .. i, function()
        ocwm.workspace.switch(i)
    end)
end
```

---

## Default Keybindings

| Keybinding | Action |
|------------|--------|
| **Super+Return** | Launch terminal (alacritty) |
| **Super+d** | Application launcher (wofi) |
| **Super+q** | Close focused window |
| **Super+1-9** | Switch to workspace 1-9 |
| **Super+Space** | Cycle through layouts |
| **Super+f** | Toggle floating mode |
| **Super+Shift+r** | Reload configuration |
| **Super+Shift+q** | Exit compositor |

All keybindings are fully customizable through `~/.config/ocwm/init.lua`.

---

## Lua API Overview

### Core Functions
```lua
ocwm.bind(keybind, callback)       -- Register keybinding
ocwm.spawn(command)                -- Launch application
ocwm.log(message)                  -- Log to compositor
ocwm.reload()                      -- Reload configuration
ocwm.on(event, callback)           -- Register event hook
```

### Window Management
```lua
ocwm.window.focused()              -- Get focused window
ocwm.window.close(window)          -- Close window
ocwm.window.list()                 -- List all windows
ocwm.window.set_floating(win, bool)
ocwm.window.set_opacity(win, 0.0-1.0)
```

### Workspace & Layout
```lua
ocwm.workspace.switch(1-9)         -- Switch workspace
ocwm.layout.set("master-stack")    -- Set layout
ocwm.layout.set_gap(pixels)        -- Configure gaps
```

### Effects
```lua
ocwm.effects.enable(bool)          -- Toggle effects
ocwm.effects.set_duration(open, close)
```

---

## Architecture

OCWM is built with a clean, modular architecture:

```
┌─────────────────────────────────┐
│     Lua Configuration Layer     │
│  (Keybinds, Rules, Behaviors)   │
└─────────────────────────────────┘
              ↕
┌─────────────────────────────────┐
│        OCWM Core (C)            │
├──────────┬──────────┬───────────┤
│  Window  │  Layout  │  Effects  │
│  Manager │  Engine  │  Pipeline │
└──────────┴──────────┴───────────┘
              ↕
┌─────────────────────────────────┐
│   wlroots Scene Graph Renderer  │
└─────────────────────────────────┘
              ↕
┌─────────────────────────────────┐
│      Wayland Protocol           │
└─────────────────────────────────┘
```

### Project Structure
```
ocwm/
├── src/
│   ├── main.c          # Server initialization
│   ├── ocwm.h          # Core data structures
│   ├── wayland/        # Output and input handling
│   ├── wm/             # Window management
│   ├── layout/         # Tiling algorithms
│   ├── effects/        # Animation engine
│   └── lua/            # Lua API
├── config/
│   └── init.lua        # Example configuration
└── Makefile            # Build system
```

---

## Performance

OCWM is designed for efficiency:

- **Binary Size**: < 700KB (release build)
- **Memory Usage**: ~30MB base, ~50MB with 20 windows
- **Startup Time**: 80-120ms cold start
- **Frame Rate**: Solid 60 FPS with animations enabled
- **Animation Overhead**: < 0.1ms per frame

---

### Code Style
- Follow existing C code style (K&R with tabs)
- Comment complex logic
- Keep functions focused and modular
- Test on multiple platforms when possible

---

## License

OCWM is released under the **MIT License**. See [LICENSE](LICENSE) for details.

---

## Acknowledgments

OCWM is built on the shoulders of giants:

- **[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)** - Wayland compositor library
- **[Lua](https://www.lua.org/)** - Powerful, lightweight scripting language
- **[Wayland](https://wayland.freedesktop.org/)** - Modern display server protocol

Inspired by:
- **dwm** - Minimalism and efficiency
- **i3** - Intuitive tiling workflow
- **Awesome WM** - Lua configuration power
- **Hyprland** - Modern visual effects

---

## Support

- **Issues**: Report bugs and request features on the [issue tracker](https://github.com/yourusername/ocwm/issues)
- **Discussions**: Join conversations in [GitHub Discussions](https://github.com/yourusername/ocwm/discussions)
- **Documentation**: See [PROJECT_COMPLETE.md](PROJECT_COMPLETE.md) for comprehensive documentation

---

**Built with passion for minimal, beautiful desktop environments.**

*OCWM - Where performance meets aesthetics.*
