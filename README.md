![UILO](git_images/uilo-logo.png)

# UILO

A lightweight, modern C++ UI library with fast, hardware-accelerated rendering powered by **bgfx** and **SDL3**.

## Overview

UILO provides a retained-mode UI system with automatic layout management, responsive containers, and a clean declarative API. Build cross-platform applications with composable widgets, flexible alignment, and event handling — all in pure C++.

**Licensed under MPL-2.0.**

## Features

- **Retained-mode UI** — Elements persist and update efficiently
- **Automatic Layout** — Responsive rows, columns, and flexible sizing
- **Hardware Accelerated** — bgfx rendering with direct GPU support
- **Cross-platform** — macOS, Linux, Windows support (SDL3 + bgfx)
- **Declarative API** — Chainable modifiers and composable layouts
- **Static & Dynamic** — Build as static or shared library
- **Integrated Build** — Vendored dependencies, one-command builds

## Build

### Requirements

- **macOS:** Xcode command-line tools (clang)
- **Linux:** gcc or clang, CMake 3.20+
- **All platforms:** Python 3 (for bgfx build)

### Quick Start

Build UILO with one of these commands:

```bash
./build.sh                  # Release, static
./build.sh debug            # Debug, static
./build.sh release dynamic  # Release, shared
./build.sh clean            # Clean build/ (preserves vendor cache)
UILO_CLEAN_EXT=1 ./build.sh # Force rebuild of dependencies
```

The build script:

- Clones and caches SDL3 at `.sdl3build/` (one-time)
- Clones and builds bgfx + tools at `ext/bgfx/` (one-time)
- Configures and builds UILO via CMake
- Outputs binaries to `build/{Release,Debug}-{static,dynamic}/`

## Architecture

UILO is structured around a few core concepts:

### Element & Widgets

All UI components inherit from `Element`. Common widgets:

- **Container**: `Column`, `Row`, `ScrollableColumn`, `ScrollableRow`
- **Input**: `Button`, `Slider`, `Dropdown`, `TextInput`
- **Display**: `Text`, `Image`, `Spacer`

### Layout System

- **Column** — Vertical stacking with height distribution
- **Row** — Horizontal layout with width distribution
- **Fixed/Percentage sizing** — Mix pixel and proportional dimensions
- **Alignment flags** — `CENTER_X`, `CENTER_Y`, `LEFT`, `RIGHT`, `TOP`, `BOTTOM`

### Modifiers

Configure appearance and behavior with chainable methods:

```cpp
Modifier()
    .setWidth(0.5f)         // 50% of parent width
    .setfixedHeight(40.f)   // 40 pixels
    .setColor(0xFF0000FF)   // RGBA color
    .align(Align::CENTER_X)
    .onLClick([]() { /* ... */ })
```

### Pages

Named UI screens that can be switched at runtime:

```cpp
app.addPage(page({/* ... */}), "main");
app.switchToPage("main");
```

## Example

```cpp
#include "UILO.hpp"

int main() {
    UILO app("My App");

    auto mainPage = page({
        column({
            text(
                Modifier().setfixedHeight(40.f).align(Align::CENTER_X),
                "Welcome to UILO",
                "path/to/font.ttf"
            ),
            button(
                Modifier()
                    .setfixedSize({150.f, 40.f})
                    .align(Align::CENTER_X)
                    .onLClick([]() { std::cout << "Clicked!\n"; }),
                ButtonStyle::Pill,
                "Click Me",
                "path/to/font.ttf"
            )
        })
    });

    app.addPage(std::move(mainPage), "main");
    app.switchToPage("main");

    while (app.isRunning()) {
        app.update();
        app.render();
    }

    return 0;
}
```

## Key APIs

### UILO (Application)

```cpp
UILO(const std::string& title);
void addPage(std::unique_ptr<Page>, const std::string& name);
void switchToPage(const std::string& name);
void update();
void render();
bool isRunning();
```

### Modifiers & Options

UILO uses two complementary configuration systems:

**Modifier** — Common layout and event handling (all widgets):

```cpp
Modifier()
    .setWidth(0.5f)              // 50% of parent width
    .setfixedHeight(40.f)        // 40 pixels
    .align(Align::CENTER_X)
    .onLClick([]() { /* ... */ })

// Key methods:
.setWidth(float pct)
.setHeight(float pct)
.setfixedWidth(float px)
.setfixedHeight(float px)
.setColor(Color c)
.setVisible(bool)
.align(Align flags)
.onLClick(std::function<void()>)
.onRClick(std::function<void()>)
```

**Options** — Widget-specific styling and behavior:

Each widget has specialized Options (ButtonOptions, SliderOptions, RowOptions, etc.):

```cpp
ButtonOptions()
    .setColor(Color::Red)
    .setRounding(8.f)

SliderOptions()
    .setTrackColor(Color::Gray)
    .setThumbColor(Color::Blue)
    .setRange(0.f, 100.f)
    .setOnValueChanged([](float v) { /* ... */ })

RowOptions()
    .setGradient({topLeft, topRight, bottomLeft, bottomRight})
    .setRounding(4.f)
    .setScrollable(true)
```

Pass both to factory functions:

```cpp
row(
    Modifier().setHeight(0.5f),      // Layout
    RowOptions().setGradient(g),     // Widget-specific styling
    {/* children */}
)
```

### Common Widgets

```cpp
Text*               text(Modifier, const std::string& str, const std::string& fontPath);
Button*             button(Modifier, ButtonStyle, const std::string& label, const std::string& fontPath);
Slider*             horizontalSlider(Modifier, uint32_t knobColor, uint32_t barColor);
Slider*             verticalSlider(Modifier, uint32_t knobColor, uint32_t barColor);
Dropdown*           dropdown(Modifier, const std::string& defaultText, std::vector<std::string> options, ...);
Image*              image(Modifier, const sf::Image&);
Column*             column(std::vector<Element*> children);
Row*                row(std::vector<Element*> children);
ScrollableColumn*   scrollableColumn(Modifier, std::vector<Element*> children);
ScrollableRow*      scrollableRow(Modifier, std::vector<Element*> children);
Spacer*             spacer(Modifier);
```

## Dependencies

UILO vendors and manages all dependencies:

- **SDL3** — Window, input, and event handling ([release-3.2.10](https://github.com/libsdl-org/SDL/releases/tag/release-3.2.10))
- **bgfx** — Rendering backend (latest from main)
- **bx/bimg** — bgfx utilities
- **stb** — Header-only image/font libraries (included in third_party/)

All dependencies are cloned into `ext/` and built once, then cached. Clean builds don't rebuild them.

## Performance

- **One-time builds** — bgfx and SDL3 build once and are cached
- **Incremental rebuilds** — Only UILO sources recompile on changes
- **Stress testing** — Benchmark suite included (`make render-stress-benchmark`)
- **Hardware accelerated** — bgfx provides direct GPU rendering with minimal overhead

## Development

```bash
# View available targets
cmake --build build/Release-static --target help

# Run a specific benchmark
cmake --build build/Release-static --target render-stress-benchmark
cmake --build build/Release-static --config Release --target render-stress-benchmark --verbose
```

## License

UILO is licensed under the Mozilla Public License 2.0 (MPL-2.0). See [LICENSE](LICENSE) for details.
