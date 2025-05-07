# UILO Design Overview

* **Project Name:** UILO (pronounced *wee-low*)
* **Technology:** Built using SFML 3.0
* **Mode:** Retained-Mode UI Framework

---

## Core Principles

* Lightweight and modular retained-mode architecture
* Declarative layout via `Row` and `Column` containers
* All UI components are `Element`-derived objects
* Memory is manually managed, ownership enforced via tracking sets
* Fluent, chainable configuration using `Modifier`

---

## Layout System

* UI is composed of nested `Row` and `Column` containers
* All layout decisions (alignment, sizing) are made by containers
* Alignment is explicit via the `Align` enum:

  * `Row`: aligns children left, center, or right horizontally
  * `Column`: aligns children top, center, or bottom vertically
* Each alignment group is laid out independently and sequentially
* Nested containers propagate layout bounds downward

### Example Layout Code

```cpp
UILO ui("My UI", {{
    new Page({

        // Page Containers
        // ...
        
        new Row
        (
            Modifier()
            .setColor(sf::Color(30, 30, 30)), 
        {
            // Row Elements
            // ...

            new Column
            (
                Modifier()
                .setfixedWidth(100)
                .align(Align::LEFT)
                .setColor(sf::Color::Red), 
            {

                new Row
                (
                    Modifier()
                    .setfixedHeight(100)
                    .align(Align::TOP)
                    .onClick([](){ std::cout << "Clicked top"; }),
                {
                    // Row Elements
                    // ...
                }),

                new Row
                (
                    Modifier()
                    .setfixedHeight(100)
                    .align(Align::CENTER_Y),
                {
                    // Row Elements
                    // ...
                })
            }),

            new Column
            (
                Modifier()
                .setfixedWidth(100)
                .align(Align::CENTER_X)
                .setColor(sf::Color::Green), 
            {
                // Column Elements
                // ...
            })
        })
    }), "main"}
});

while (ui.isRunning()) {
    ui.update();
    ui.render();
}
```

---

## Modifier System

All configuration of an element's layout, appearance, and behavior is done through a `Modifier`.

### Modifier Options:

* `.setWidth(float pct)` / `.setfixedWidth(float px)`
* `.setHeight(float pct)` / `.setfixedHeight(float px)`
* `.align(Align::...)` — alignment enum (bitmask)
* `.setColor(sf::Color)` — background fill
* `.onClick(func)` — lambda or callback function
* `.setVisible(bool)` — toggles element visibility

### Example:

```cpp
Modifier()
.setfixedWidth(100)
.setfixedHeight(50)
.align(Align::TOP | Align::CENTER_X)
.setColor(sf::Color::Red)
.onClick([]() { std::cout << "Clicked!"; });
```

---

## Event Handling

* Events are handled internally by `UILO`
* Clicks are routed top-down to visible elements
* If an element's bounds contain the click, it triggers its `.onClick()`
* Pages and containers relay events to children

---

## Sizing Behavior

* Default size: 100% of parent bounds
* Fixed size overrides percentage
* Containers pass their bounds to children during layout

---

## Alignment Rules

`Align` is a bitmask enum specifying layout placement:

* Vertical: `TOP`, `CENTER_Y`, `BOTTOM`
* Horizontal: `LEFT`, `CENTER_X`, `RIGHT`

Examples:

* `Align::TOP | Align::CENTER_X` = centered horizontally at the top
* `Align::BOTTOM` = bottom-aligned, default horizontal stack order

---

## Memory and Ownership

* Elements and pages are created with `new` and passed to `UILO`
* Ownership is transferred and tracked via internal sets
* Double deletion or reuse is detected and aborts the program
* Pointers passed to `UILO::addPage()` are nulled out after transfer

---

## Class Hierarchy

```
       Element (abstract base class)
                 |
       +---------+---------+
       |                   |
   Container            Control (planned)
       |
   +---+---+
   |       |
Column    Row
```