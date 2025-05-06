
---

# Uilo Design Overview

* **Project Name:** Uilo (pronounced *wee-low*)
* **Technology:** Built using SFML 3.0
* **Mode:** Retained-Mode UI Framework

---

## Core Principles

* Lightweight and modular
* Retained-mode architecture
* Object-based UI elements
* Designed for reusability and layout composability
* Fluent, chainable configuration using `Modifier` objects
* Manual memory model (raw pointers managed via destructors)

---

## Layout System

* Based on `Column` and `Row` containers
* All layout is handled via container logic; elements do not self-align
* Alignment handled per-axis:

  * `Column` handles vertical alignment (`TOP`, `CENTER_Y`, `BOTTOM`)
  * `Row` will handle horizontal alignment (`LEFT`, `CENTER_X`, `RIGHT`)
* Nested containers supported, with full propagation of bounds

### Example Layout Code

```cpp
uilo::UILO ui("My UI", {
    {new uilo::Page({
        // Base Container
        {new uilo::Column(
            Modifier().setfixedWidth(100).align(Align::LEFT), {
            // Add Rows/Columns/Other elements here

            {new uilo::Row(
                Modifier().setfixedHeight(50).align(Align::TOP), {
                  // Add Rows/Columns/Other elements here
                }
            )},

            {new uilo::Row(
                Modifier().setfixedHeight(50).align(Align::BOTTOM), {
                  // Add Rows/Columns/Other elements here
                }
            )}
        })}
    }), "main"}
});

while (ui.isRunning()) {
    ui.update();
    ui.render();
}
```

---

## Modifier System

All layout and visual configuration is done via `Modifier`, a lightweight chainable struct.

### Supported Modifier Settings:

* `.setWidth(float pct)` / `.setfixedWidth(float px)`
* `.setHeight(float pct)` / `.setfixedHeight(float px)`
* `.align(Align::...)` — bitmask enum for layout alignment
* `.setColor(sf::Color)` — fill color for background

### Example:

```cpp
Modifier()
    .setfixedWidth(100)
    .setfixedHeight(50)
    .align(Align::TOP | Align::CENTER_X)
    .setColor(sf::Color::Red);
```

---

## Alignment

* Bitmask enum `Align` controls element placement
* Supported flags:

  * `TOP`, `BOTTOM`, `CENTER_Y`
  * `LEFT`, `RIGHT`, `CENTER_X`
* Containers organize children by their vertical/horizontal alignment
* Each alignment group is laid out independently and sequentially:

  * `TOP`: stacked from top down
  * `BOTTOM`: stacked from bottom up (preserving insertion order)
  * `CENTER_Y`: centered as a block

---

## Sizing

* Fixed size overrides percentage-based size
* Default sizing is full (100%) width/height of parent if unspecified
* All sizing is handled relative to parent bounds in containers

---

## Element Structure

All UI components derive from `Element`, which contains:

* `sf::RectangleShape m_bounds` — current size and position
* `Modifier m_modifier` — layout metadata
* `virtual update()` / `render()` methods

Containers inherit from `Element` and manage child elements.

---

## Class Hierarchy

```
               Element (abstract base class)
                      |
         ┌────────────┴────────────┐
         |                         |
     Container                  Control (planned)
         |
  ┌──────┴───────────────┐
  |                      |
Column                  Row
```

### Descriptions:

* **Element**: Base class with bounds and modifiers
* **Container**: Composite layout elements that manage children
* **Column/Row**: Vertical and horizontal layout respectively
* **Control** (planned): Interactive elements like Label, Button

---

## UI Elements (Current & Planned)

* ✅ Column
* ✅ Row
* ⏳ Text (in progress)
* ⏳ Spacer
* ⏳ Label
* ⏳ Button
* ⏳ Panel

---

## Styling/Theming (Planned)

* Per-element color already supported via `.setColor()`
* Planned support for fonts, padding, margins via `Modifier` or theme object

---

## Event Handling (Planned)

* Event queue (`uilo::Events`) exists for user-defined interaction
* Window events handled internally
* Focused input, hover, and clicks will be routed top-down

---

## Intended Use Case

* UI system for SFML-based apps, tools, or games
* Emphasis on simple integration and explicit control over layout
* Encourages clean, declarative UI construction

---

## Long-Term Goals

* Simple, clean retained-mode UI with minimal dependencies
* Extensible layout and rendering primitives
* Support for animation, transitions, and hover states (future)
* Optional tooling for layout preview/testing

---

## Current Development Focus

* ✅ Vertical layout with alignment logic
* ✅ Sizing system with fixed and percent support
* ✅ Internal window management via `UILO`
* ⏳ Row layout and horizontal alignment
* ⏳ Basic interactive elements and event routing

---

Let me know if you'd like me to format this into a `.md` file for GitHub or help generate badges (e.g. `build`, `license`, `SFML 3.0 compatible`).