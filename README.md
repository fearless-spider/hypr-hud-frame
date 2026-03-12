# hypr-hud-frame

A Hyprland plugin that renders cyberpunk-style HUD frames around windows. Chamfered corners, neon glow, hash marks, accent decorations, and pulse animations — all built with `CRectPassElement` for maximum stability.

![License](https://img.shields.io/badge/license-MIT-blue)

## Features

- **Neon border** with active/inactive color switching
- **Chamfered (cut) corners** with dark fill — top-left and bottom-right diagonals
- **Glow effect** — concentric rect layers with quadratic alpha falloff
- **Pulse animation** — breathing glow on active windows
- **Hash marks** — diagonal accent lines at corners
- **Edge decorations** — dots, accent bars, inner accent lines
- **Padding** — configurable gap between window edge and border
- **Toggle** — enable/disable with a keybinding
- **Per-class exclusion** — skip specific window classes (e.g. launchers)

## Requirements

- Hyprland (with plugin support)
- CMake 3.19+
- C++23 compiler

## Installation

### With hyprpm

```bash
hyprpm add https://github.com/fearless-spider/hypr-hud-frame
hyprpm enable hypr-hud-frame
```

### Manual build

```bash
git clone https://github.com/fearless-spider/hypr-hud-frame.git
cd hypr-hud-frame
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Then load the plugin:

```bash
mkdir -p dist
cp build/hypr-hud-frame.so dist/
hyprctl plugin load $(pwd)/dist/hypr-hud-frame.so
```

### Autoload

Add to your `hyprland.conf`:

```ini
plugin = /path/to/hypr-hud-frame/dist/hypr-hud-frame.so
```

## Configuration

Add to your `hyprland.conf`:

```ini
plugin {
    hypr-hud-frame {
        # Geometry
        border_width = 3
        glow_radius = 12
        notch_size = 18
        padding = 4

        # Colors (RGBA hex)
        col.active_border = 0xaf6df9cc
        col.inactive_border = 0x3b345530
        col.glow = 0xaf6df944
        col.glow_inactive = 0x3b345511

        # Animation
        pulse_speed = 2.5
        pulse_intensity = 0.35

        # Feature toggles
        show_edge_decor = 1
        show_hash_marks = 1
        show_corner_notch = 1
        enabled = 1

        # Exclude window classes (comma-separated)
        exclude_classes = wofi,polkit-gnome-authentication-agent-1
    }
}
```

### Toggle keybinding

```ini
bind = $mainMod SHIFT, F, hypr-hud-frame:toggle,
```

## Configuration reference

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `border_width` | int | 2 | Border line thickness in pixels |
| `glow_radius` | int | 10 | Outer glow spread radius |
| `notch_size` | int | 8 | Corner cut size (0 = square corners) |
| `padding` | int | 0 | Gap between window edge and border |
| `pulse_speed` | float | 2.0 | Pulse animation speed |
| `pulse_intensity` | float | 0.12 | Pulse brightness amplitude |
| `show_edge_decor` | bool | 1 | Show dots and accent bars |
| `show_hash_marks` | bool | 1 | Show diagonal hash marks at corners |
| `show_corner_notch` | bool | 1 | Show chamfered corner clips |
| `enabled` | bool | 1 | Enable/disable rendering |
| `col.active_border` | hex | 0x61E2FFCC | Active window border color |
| `col.inactive_border` | hex | 0x6B5F7B66 | Inactive window border color |
| `col.glow` | hex | 0x61E2FF44 | Active glow color |
| `col.glow_inactive` | hex | 0x6B5F7B11 | Inactive glow color |
| `exclude_classes` | string | "" | Comma-separated window classes to skip |

## Development notes

**Important:** Never overwrite a loaded `.so` file — Hyprland memory-maps it and will crash. Always unload first:

```bash
hyprctl plugin unload /path/to/hypr-hud-frame.so
cmake --build build -j$(nproc)
cp build/hypr-hud-frame.so dist/hypr-hud-frame.so
hyprctl plugin load /path/to/dist/hypr-hud-frame.so
```

## License

MIT — see [LICENSE](LICENSE) for details.

## Author

**Spider's LAB**
