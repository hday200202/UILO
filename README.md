# Uilo Design Overview

**Project Name:** Uilo (pronounced *wee-low*)  
**Technology:** Built using SFML  
**Mode:** Retained-Mode UI Framework

---

## Core Principles
- Lightweight and modular
- Retained-mode architecture
- Object-based UI elements
- Designed for reusability and layout composability

---

## Layout System
- Based on `Column` and `Row` containers
- Supports nested layout composition
- Elements are added to layout containers (e.g. `column.add(label)`)

## Alignment
- Alignment flags for both horizontal and vertical alignment:
  - Horizontal: `left_align`, `center_align`, `right_align`
  - Vertical: `top_align`, `middle_align`, `bottom_align`
- Nested alignment supported within parent containers

## Sizing
- Supports both fixed and percentage-based sizing
- Percentage sizing uses values between 0.0 and 1.0
- Allows for responsive UI that adapts to container dimensions

## Element Structure
- All UI elements inherit from a base class (e.g. `Element`)
- Each element can have:
  - Position
  - Size (fixed or percentage)
  - Alignment flags
  - Event handling (optional)

---

## Class Hierarchy

```
               Element (abstract base class)
                      |
    ┌─────────────────┴────────────────────┐
    |                 |                    |
  Container        Control               Spacer
    |                 |
    |         ┌───────┴─────────┐
    |       Label             Button
    |
┌───┴───────────────┐
|                   |
Row               Column
```

### Descriptions
- **Element**: Base for all components; holds position, size, alignment, etc.
- **Container**: Can hold and lay out children (Row, Column)
- **Control**: Interactive elements like buttons or labels
- **Spacer**: Invisible element for spacing
- **Row/Column**: Layout children horizontally or vertically
- **Label/Button**: Basic UI controls

---

## UI Elements (Planned)
- Label
- Button
- Panel (for grouping elements)
- Spacer
- Row / Column
- More can be added modularly

## Styling/Theming
- Global or per-element theme struct (colors, padding, font, etc.) is planned
- Lightweight theming system to control visuals uniformly

## Focus and Input
- Event propagation planned through layout hierarchy
- Focus handling for keyboard interaction may be supported in future

## Intended Use Case
- Primarily for SFML-based applications and games
- Designed to be developer-friendly and easy to integrate

## Long-Term Goals
- Clean, consistent API
- Minimal dependencies beyond SFML
- Support for animated transitions or hover/click effects (later phase)

---

## Current Development Focus
- Establishing architecture and layout logic
- Prioritizing basic containers and alignment
- No immediate-mode features; strictly retained-mode

**Developer Note:**  
This project is in early planning. Code structure and interfaces will follow once the overall design is finalized.  
Designed with future extensibility in mind, while keeping the core simple.
