--[[
    OCWM Configuration File

    This file is loaded on startup and can be hot-reloaded with Mod+Shift+R
    Customize your window manager with Lua!
--]]

ocwm.log("Loading OCWM configuration...")

--
-- Effect Settings
--

-- Enable animations and effects
ocwm.effects.enable(true)

-- Set animation durations (in milliseconds)
ocwm.effects.set_duration(250, 200)  -- 250ms open, 200ms close

--
-- Keybindings
--

-- Terminal
ocwm.bind("Mod+Return", function()
    ocwm.spawn("alacritty")
end)

-- Alternative terminals
ocwm.bind("Mod+Shift+Return", function()
    ocwm.spawn("foot")
end)

-- Application launcher (rofi/wofi)
ocwm.bind("Mod+d", function()
    ocwm.spawn("wofi --show drun")
end)

ocwm.bind("Mod+Shift+d", function()
    ocwm.spawn("rofi -show drun")
end)

-- Window management
ocwm.bind("Mod+q", function()
    local win = ocwm.window.focused()
    if win then
        ocwm.window.close(win)
        ocwm.log("Closing focused window")
    end
end)

-- Toggle floating for focused window
ocwm.bind("Mod+f", function()
    local win = ocwm.window.focused()
    if win then
        ocwm.window.set_floating(win, true)
        ocwm.log("Window set to floating")
    end
end)

ocwm.bind("Mod+t", function()
    local win = ocwm.window.focused()
    if win then
        ocwm.window.set_floating(win, false)
        ocwm.log("Window set to tiled")
    end
end)

-- Layout switching
ocwm.bind("Mod+space", function()
    local current = ocwm.layout.get()
    ocwm.log("Current layout: " .. current)

    -- Cycle through layouts
    if current == "floating" then
        ocwm.layout.set("master-stack")
    elseif current == "master-stack" then
        ocwm.layout.set("grid")
    elseif current == "grid" then
        ocwm.layout.set("monocle")
    else
        ocwm.layout.set("floating")
    end

    ocwm.log("Switched to: " .. ocwm.layout.get())
end)

-- Specific layouts
ocwm.bind("Mod+Shift+f", function()
    ocwm.layout.set("floating")
    ocwm.log("Layout: floating")
end)

ocwm.bind("Mod+Shift+t", function()
    ocwm.layout.set("master-stack")
    ocwm.log("Layout: master-stack")
end)

ocwm.bind("Mod+Shift+g", function()
    ocwm.layout.set("grid")
    ocwm.log("Layout: grid")
end)

ocwm.bind("Mod+Shift+m", function()
    ocwm.layout.set("monocle")
    ocwm.log("Layout: monocle")
end)

-- Adjust master ratio (for master-stack layout)
ocwm.bind("Mod+h", function()
    ocwm.layout.set_master_ratio(0.5)
    ocwm.log("Master ratio: 50%")
end)

ocwm.bind("Mod+l", function()
    ocwm.layout.set_master_ratio(0.6)
    ocwm.log("Master ratio: 60%")
end)

-- Adjust gaps
ocwm.bind("Mod+minus", function()
    ocwm.layout.set_gap(5)
    ocwm.log("Gap: 5px")
end)

ocwm.bind("Mod+equal", function()
    ocwm.layout.set_gap(15)
    ocwm.log("Gap: 15px")
end)

-- Workspace switching (1-9)
for i = 1, 9 do
    ocwm.bind("Mod+" .. i, function()
        ocwm.workspace.switch(i)
        ocwm.log("Switched to workspace " .. i)
    end)
end

-- Applications
ocwm.bind("Mod+w", function()
    ocwm.spawn("firefox")
end)

ocwm.bind("Mod+e", function()
    ocwm.spawn("thunar")
end)

-- Reload configuration
ocwm.bind("Mod+Shift+r", function()
    ocwm.log("Reloading configuration...")
    ocwm.reload()
end)

-- Exit compositor
ocwm.bind("Mod+Shift+q", function()
    ocwm.log("Exiting OCWM...")
    ocwm.quit()
end)

-- Screenshot
ocwm.bind("Print", function()
    ocwm.spawn("grim ~/screenshot-$(date +%Y%m%d-%H%M%S).png")
    ocwm.log("Screenshot saved")
end)

ocwm.bind("Shift+Print", function()
    ocwm.spawn("grim -g \"$(slurp)\" ~/screenshot-$(date +%Y%m%d-%H%M%S).png")
    ocwm.log("Screenshot region saved")
end)

-- Lock screen
ocwm.bind("Mod+l", function()
    ocwm.spawn("swaylock -f -c 000000")
end)

-- Volume controls (if you have wpctl/pactl)
ocwm.bind("XF86AudioRaiseVolume", function()
    ocwm.spawn("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%+")
end)

ocwm.bind("XF86AudioLowerVolume", function()
    ocwm.spawn("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-")
end)

ocwm.bind("XF86AudioMute", function()
    ocwm.spawn("wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle")
end)

-- Brightness controls
ocwm.bind("XF86MonBrightnessUp", function()
    ocwm.spawn("brightnessctl set +5%")
end)

ocwm.bind("XF86MonBrightnessDown", function()
    ocwm.spawn("brightnessctl set 5%-")
end)

--
-- Event Hooks
--

-- Window opened
ocwm.on("window_open", function(window)
    local title = ocwm.window.get_title(window)
    local app_id = ocwm.window.get_app_id(window)

    ocwm.log("Window opened: " .. app_id .. " - " .. title)

    -- Example: Custom effects for specific applications
    if app_id == "firefox" then
        ocwm.log("Firefox detected!")
        ocwm.window.set_opacity(window, 0.98)  -- Slightly transparent
        -- ocwm.window.set_blur(window, true)  -- Enable blur (expensive!)

    elseif app_id == "Alacritty" or app_id == "alacritty" then
        ocwm.log("Terminal opened!")
        ocwm.window.set_opacity(window, 0.95)  -- Terminal transparency
        -- ocwm.window.set_blur(window, true)  -- Blur behind terminal

    elseif app_id == "thunar" or app_id == "nemo" or app_id == "nautilus" then
        -- File managers look nice with slight transparency
        ocwm.window.set_opacity(window, 0.96)
    end
end)

-- Window closed
ocwm.on("window_close", function(window)
    local title = ocwm.window.get_title(window)
    ocwm.log("Window closed: " .. title)
end)

-- Window focused
ocwm.on("window_focus", function(window)
    local title = ocwm.window.get_title(window)
    local app_id = ocwm.window.get_app_id(window)
    ocwm.log("Focus: " .. app_id)
end)

--
-- Utility Functions
--

-- List all windows
function list_windows()
    local windows = ocwm.window.list()
    ocwm.log("Open windows: " .. #windows)
    for i, win in ipairs(windows) do
        local title = ocwm.window.get_title(win)
        local app_id = ocwm.window.get_app_id(win)
        ocwm.log("  [" .. i .. "] " .. app_id .. ": " .. title)
    end
end

-- Keybind to list windows
ocwm.bind("Mod+i", list_windows)

--
-- Startup Applications
--

-- Uncomment to auto-start applications
-- ocwm.spawn("waybar")
-- ocwm.spawn("mako")
-- ocwm.spawn("nm-applet --indicator")

ocwm.log("✓ OCWM configuration loaded successfully!")
ocwm.log("✓ Keybindings active")
ocwm.log("✓ Event hooks registered")
ocwm.log("✓ Workspaces and layouts initialized")
ocwm.log("✓ Effects and animations enabled")
ocwm.log("")
ocwm.log("Quick Keys:")
ocwm.log("  Mod+Return       - Terminal")
ocwm.log("  Mod+d            - App launcher")
ocwm.log("  Mod+q            - Close window")
ocwm.log("  Mod+1-9          - Switch workspace")
ocwm.log("  Mod+Space        - Cycle layouts")
ocwm.log("  Mod+f            - Float window")
ocwm.log("  Mod+t            - Tile window")
ocwm.log("  Mod+Shift+r      - Reload config")
ocwm.log("  Mod+Shift+q      - Exit OCWM")
ocwm.log("")
ocwm.log("Layouts: floating, master-stack, grid, monocle")
ocwm.log("Effects: Window animations, opacity, blur (configurable)")
