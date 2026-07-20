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
- **Declarative API** — Chainable Modifiers and Options, composable layouts
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
./build.sh                # Release, static
./build.sh debug          # Debug, static
./build.sh release dynamic # Release, shared
./build.sh clean          # Clean build/ (preserves vendor cache)
UILO_CLEAN_EXT=1 ./build.sh # Force rebuild of dependencies
```

The build script:

- Clones and caches SDL3 at `.sdl3build/` (one-time)
- Clones and builds bgfx + tools at `ext/bgfx/` (one-time)
- Configures and builds UILO via CMake
- Outputs binaries to `build/{Release,Debug}-{static,dynamic}/`

## Core Concepts

### Modifier — Layout & Events

Every element takes a `Modifier` for common configuration: sizing, alignment, callbacks, padding, and visibility.

```cpp
Modifier()
    .setWidth(50_pct)         // 50% of parent width
    .setHeight(100_px)        // 100 pixels
    .setAlign(Align::Center)
    .setOnLeftClick([](Button* btn) { /* ... */ })
    .setOnHoverEnter([](Element* el) { /* ... */ })
    .setOuterPadding(8.f)
    .setVisible(true)
```

### Options — Widget-Specific Styling

Each widget has dedicated Options for its own properties: colors, fonts, gradients, scrolling, ranges, etc.

```cpp
TextOptions()
    .setFont("fonts/regular.ttf")
    .setContent("Hello World")
    .setCharSize(16)
    .setColor(Color::White)
    .setWrap(true)
    .setBold(true)

ButtonOptions()
    .setColor(Color::Blue)
    .setGradient(Gradient().setTop(topColor).setBottom(bottomColor))
    .setRounding(8.f)
    .setLabel(labelText)

SliderOptions()
    .setTrackColor(Color::Gray)
    .setThumbColor(Color::White)
    .setRange(0.f, 100.f)
    .setStep(1.f)
    .setOrientation(SliderOrientation::Horizontal)
    .setOnValueChanged([](Slider* s, float v) { /* ... */ })

RowOptions()
    .setColor(Color::Panel)
    .setGradient(Gradient().setLeft("accent").setRight("panel"))
    .setRounding(4.f)
    .setScrollable(true)
    .setScrollSpeed(20.f)
```

### Gradients

A `Gradient` is a per-corner background fill. Each corner takes a `Color` **or** a
palette role name (resolved at draw time, so switching palettes restyles gradients
automatically) — the two are freely mixable. Fluent setters name the position each
color occupies:

```cpp
// Vertical fade (top -> bottom)
Gradient().setTop(Color{97, 62, 180}).setBottom(Color{34, 27, 58})

// Horizontal, driven by palette roles (follows the active theme)
Gradient().setLeft("accent").setRight("panel")

// Four explicit corners
Gradient()
    .setTopLeft(c1).setTopRight(c2)
    .setBottomLeft(c3).setBottomRight(c4)

// Mixed literal color + role
Gradient().setTop(Color::Red).setBottom("panel")
```

`setTop` / `setBottom` / `setLeft` / `setRight` each set both corners of that edge;
the four corner setters give full control. Apply a gradient with
`setGradient(...)` on any container/button Options; it composes with `setRounding`.

Gradients can also be named once in the `Palette` and referenced per element:

```cpp
palette.setGradient("hero", Gradient().setTop(Color{97, 62, 180})
                                      .setBottom(Color{34, 27, 58}));
row(Modifier(), RowOptions().setGradientRole("hero"));
```

### Factory Functions

Create elements using simple factory functions. All take `Modifier` and widget-specific `Options`:

```cpp
text(Modifier(), TextOptions().setContent("Label"), "label_name");
button(Modifier().setOnLeftClick([](Button* b) { }), ButtonOptions());
slider(Modifier(), SliderOptions().setRange(0.f, 1.f));
column(Modifier(), ColumnOptions(), {child1, child2, child3}, "my_column");
row(Modifier(), RowOptions().setGradient(g), {child1, child2});
```

### Dimension Literals

Sizes use `Dimension` with convenient syntax:

```cpp
50_pct    // 50% of parent
100_px    // 100 pixels
```

### Containers

Stack and arrange elements with automatic layout:

```cpp
column(
    Modifier().setHeight(100_pct),
    ColumnOptions(),
    {
        text(Modifier().setHeight(20_pct), TextOptions().setContent("Header")),
        row(Modifier().setHeight(60_pct), RowOptions(), {/*children*/}),
        spacer(Modifier().setHeight(20_pct))
    }
)
```

## Example

```cpp
#include "UILO.hpp"

int main() {
    UILO app("My App");

    auto mainPage = page(
        column(
            Modifier(),
            ColumnOptions(),
            {
                text(
                    Modifier().setHeight(50_px).setAlign(Align::CenterX),
                    TextOptions()
                        .setFont("fonts/regular.ttf")
                        .setContent("Welcome to UILO")
                        .setCharSize(24),
                    "header"
                ),
                button(
                    Modifier()
                        .setHeight(40_px)
                        .setWidth(150_px)
                        .setAlign(Align::CenterX)
                        .setOnLeftClick([](Button* btn) { 
                            std::cout << "Clicked!\n"; 
                        }),
                    ButtonOptions()
                        .setColor(Color::Blue)
                        .setRounding(4.f),
                    "click_me"
                )
            },
            "main_column"
        ),
        "main"
    );

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

### Modifier Methods

```cpp
.setWidth(Dimension)
.setHeight(Dimension)
.setAlign(Align)
.setOnLeftClick(callback)
.setOnRightClick(callback)
.setOnHoverEnter(callback)
.setOnHoverExit(callback)
.setOnScroll(callback)
.setOnUpdateStart(callback)
.setOnUpdateEnd(callback)
.setOuterPadding(float)
.setVisible(bool)
.ignoreScroll(bool)
.setFreePosition(Vec2f)
.setMaterial(Material)
```

### Common Widget Factories

```cpp
Column*    column(Modifier, ColumnOptions, contains children, const std::string& name);
Row*       row(Modifier, RowOptions, contains children, const std::string& name);
Canvas*    canvas(Modifier, CanvasOptions, contains children, const std::string& name);
Text*      text(Modifier, TextOptions, const std::string& name);
Button*    button(Modifier, ButtonOptions, const std::string& name);
Slider*    slider(Modifier, SliderOptions, const std::string& name);
Knob*      knob(Modifier, KnobOptions, const std::string& name);
Dropdown*  dropdown(Modifier, DropdownOptions, std::initializer_list<std::string> items, const std::string& name);
Image*     image(Modifier, ImageOptions, const std::string& name);
Spacer*    spacer(Modifier, SpacerOptions, const std::string& name);
Textbox*   textbox(Modifier, TextboxOptions, const std::string& name);
Resizer*   resizer(Modifier, ResizerOptions, const std::string& name);
Waveform*  waveform(Modifier, WaveformOptions, const std::string& name);

// Floating elements (positioned in window space, outside layout)
FreeElement freeColumn(Modifier, ColumnOptions, contains children, const std::string& name);
FreeElement freeRow(Modifier, RowOptions, contains children, const std::string& name);
```

### Common Options

**TextOptions:**

```cpp
.setFont(path)
.setContent(string)
.setCharSize(unsigned int)
.setColor(Color)
.setColorRole(string)
.setWrap(bool)
.setBold(bool)
.setItalic(bool)
.setUnderlined(bool)
.setStrikeThrough(bool)
.setTextAlignX(Align)
.setTextAlignY(Align)
```

**ButtonOptions:**

```cpp
.setColor(Color)
.setColorRole(string)
.setGradient(Gradient)
.setGradientRole(string)
.setRounding(float)
.setLabel(Text*)
```

**SliderOptions:**

```cpp
.setTrackColor(Color)
.setTrackColorRole(string)
.setFillColor(Color)
.setFillColorRole(string)
.setThumbColor(Color)
.setThumbColorRole(string)
.setThumbShape(ThumbShape)
.setTrackThickness(float)
.setTrackRounding(float)
.setThumbSize(float width, float height)
.setThumbRounding(float)
.setRange(float min, float max)
.setStep(float)
.setInvertScroll(bool)
.setDefaultValue(float)
.setOnValueChanged(callback)
.setOrientation(SliderOrientation)
```

**RowOptions / ColumnOptions:**

```cpp
.setColor(Color)
.setColorRole(string)
.setGradient(Gradient)
.setGradientRole(string)
.setRounding(float)
.setScrollable(bool)
.setScrollSpeed(float)
.setScrollMin(float)
.setScrollMax(float)
.setScrollLink(string)
.setSubDivisions(float)
.setSubDivisionMajor(unsigned int)
```

## Dependencies

UILO vendors and manages all dependencies:

- **SDL3** — Window, input, and event handling ([release-3.2.10](https://github.com/libsdl-org/SDL/releases/tag/release-3.2.10))
- **bgfx** — Rendering backend (latest from main)
- **bx/bimg** — bgfx utilities
- **stb** — Header-only image/font libraries

All dependencies are cloned into `ext/` and built once, then cached. Clean builds don't rebuild them.

## Performance

- **One-time builds** — bgfx and SDL3 build once and are cached
- **Incremental rebuilds** — Only UILO sources recompile on changes
- **Stress testing** — Benchmark suite included
- **Hardware accelerated** — bgfx provides direct GPU rendering with minimal overhead

## License

UILO is licensed under the Mozilla Public License 2.0 (MPL-2.0). See [LICENSE](LICENSE) for details.
