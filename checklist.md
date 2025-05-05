# Uilo Development Checklist

## Phase 1 – Core Architecture
- [X] Define and implement the `Element` base class
  - [X] `position`, `size`, `alignment`
  - [X] Virtual `update()`, `draw()`, and `handleEvent()`
  - [X] `onClick`, `onHover`, `onRelease` callbacks
- [ ] Create the `Container` class
  - [ ] Store a list of `Element*`
  - [ ] Add `addElement()` (single + initializer list)
  - [ ] Support method chaining (return `*this`)
- [ ] Build basic `Row` and `Column` containers
  - [ ] Implement `layout()` functions for each
  - [ ] Auto-place children based on size/alignment
- [ ] Add the `Spacer` and `Text` classes
  - [ ] `Spacer`: invisible, size-only element
  - [X] `Text`: use `sf::Text` to render simple labels

## Phase 2 – Rendering + Input
- [ ] Implement `draw(sf::RenderTarget&)` in `Element` and propagate down
- [ ] Implement `handleEvent(sf::Event)` in `Element`
  - [ ] Detect clicks (basic bounding box check)
  - [ ] Trigger `onClick` if clicked
- [ ] Build the `Button` class
  - [ ] Looks like a rounded rectangle or colored box
  - [ ] Text label in the center
  - [ ] Clickable via `onClick`
- [ ] Build the `TextField` class (stub first)
  - [ ] Placeholder for future keyboard support

## Phase 3 – Composability
- [ ] Finish `View` class
  - [ ] Holds multiple `Element*`
  - [ ] Can update, draw, and handle events
  - [ ] Optional: visibility toggle
- [ ] Create a sample app with 2–3 different views (pages/tabs)
  - [ ] Home screen with a button
  - [ ] Settings screen
  - [ ] Toggle between them with a button

## Phase 4 – Polish and Dev-Use
- [ ] Add alignment helper methods (`alignTop()`, `fillMaxWidth()`, etc.)
- [ ] Add theming system stub (colors, fonts, padding)
- [ ] Add debug draw mode (outline elements for layout testing)
- [ ] Add support for nested rows/columns

## Bonus (Future Ideas)
- [ ] Rounded rectangle utility (shared between UI elements)
- [ ] Animation helpers (e.g., fade in/out, smooth resizing)
- [ ] Style struct you can pass to elements (`uilo::Theme`)
- [ ] Smart pointer cleanup (unique_ptr for ownership safety)
