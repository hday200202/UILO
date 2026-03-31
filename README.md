![UILO](git_images/uilo-logo.png)

-----

UILO is a lightweight, header-only **retained-mode** UI library for C++ and SFML. It provides simple, composable layout primitives including rows, columns, buttons, sliders, and text. The library features a declarative API controlled via method chaining with modifiers and alignment flags.

Licensed under **MPL-2.0**.

---

## Table of Contents

  * [Getting Started](#getting-started)
  * [Core Concepts](#core-concepts)
  * [Layouts](#layouts)
  * [Widgets](#widgets)
  * [Advanced Usage](#advanced-usage)
  * [API Reference](#api-reference) 

---

### Core Philosophy

UILO is designed with the following principles in mind:

  * **Declarative & Readable:** UI structure should be easy to read and understand. Layouts are defined by nesting containers like `Row` and `Column`.
  * **Automatic Layout:** Elements are sized and positioned automatically based on their parent container and their assigned `Modifier`. This makes creating responsive UIs simple.
  * **Chainable Modifiers:** A powerful `Modifier` class allows you to chain configuration calls (`.setWidth(0.5f).setColor(sf::Color::Blue).align(...)`) for clean and concise code.
  * **Simplified Memory Management:** UILO manages the lifetime of all UI elements. You create them, and the library handles their destruction, preventing common memory leaks.

-----

## Getting Started

This section demonstrates how to create a simple application with a title and a clickable button.

### The Basic Structure

A UILO application follows these fundamental steps:

1.  **Create a `UILO` instance.** This object manages the window, events, and the main loop.
2.  **Create a `Page`.** A page is a collection of containers that represents a single screen or view.
3.  **Define a layout** using `Column` and `Row` containers.
4.  **Create widgets** (`Button`, `Text`, etc.) and add them to layout containers.
5.  **Add the `Page` to the `UILO` instance.**
6.  **Run the main loop** by calling `app.update()` and `app.render()`.

### Complete Usage Example

This example creates a window with centered text and a button that prints to the console when clicked.

```cpp
#include "UILO.hpp" // Ensure UILO.hpp is in the project directory
#include <iostream>

int main() {
    // 1. Create the main application object
    uilo::UILO app("My First UILO App");

    // 2. Create a Page using the page factory function
    auto myFirstPage = uilo::page({
        // 3. Define the layout: a single column to stack elements vertically
        uilo::column({
            // 4. Create widgets and add them to the column
            uilo::text(
                uilo::Modifier().setHeight(0.2f).setColor(sf::Color::White).align(uilo::Align::CENTER_X | uilo::Align::CENTER_Y),
                "Welcome to UILO!",
                "path/to/your/font.ttf" // IMPORTANT: This must be a valid font path
            ),
            uilo::button(
                uilo::Modifier()
                    .setfixedHeight(50.f) // 50 pixels tall
                    .setWidth(0.3f)       // 30% of the column's width
                    .setColor(sf::Color(100, 100, 100))
                    .align(uilo::Align::CENTER_X)
                    .onLClick([]() {
                        std::cout << "Button was clicked!" << std::endl;
                    }),
                uilo::ButtonStyle::Pill,
                "Click Me!",
                "path/to/your/font.ttf" // IMPORTANT: This must be a valid font path
            )
        })
    });

    // 5. Add the page to the application and assign it a name
    app.addPage(std::move(myFirstPage), "main");
    app.switchToPage("main");

    // 6. Run the main application loop
    while (app.isRunning()) {
        app.update(); // Process events and update layout
        app.render(); // Draw the UI
    }

    return 0;
}
```

-----

## Core Concepts

Understanding these core components is essential for using UILO effectively.

### Memory Management: The `obj<T>()` Factory

UILO elements should not be created with the `new` keyword. Instead, the library provides a global factory function, `uilo::obj<T>()`, and corresponding convenience wrappers.

```cpp
// Creation via the primary factory template
uilo::Button* myButton = uilo::obj<uilo::Button>(...);

// Creation via the recommended convenience function
auto myButton = uilo::button(...);
```

**Mechanism:** The `obj<T>()` function creates a `std::unique_ptr<T>` and stores it in a global vector (`uilo_owned_elements`). This transfers ownership of the element to the UILO library, which guarantees automatic memory deallocation. A raw pointer (`T*`) is returned for immediate interaction.

**Rule:** **Never call `delete` on a pointer returned by a UILO factory function.** The library manages the object's lifetime.

### The `Element` Class: The Building Block of All UI

`Element` is the abstract base class from which all UI objects, including `Button`, `Text`, `Row`, and `Column`, are derived. It defines the common interface for:

  * **Size and Position:** Stored in the `m_bounds` member.
  * **Updating:** The `update()` method recalculates layout.
  * **Rendering:** The `render()` method draws the element.
  * **Event Handling:** Virtual methods like `handleEvent()`, `checkClick()`, and `checkHover()`.
  * **Styling:** Every `Element` contains a `Modifier` object.

### The `Modifier` Class: Styling and Configuration

The `Modifier` is the primary mechanism for customization in UILO. It provides a chainable interface for configuring an element's appearance, layout, and behavior.

```cpp
uilo::Modifier()
    .setWidth(0.8f) // 80% of parent's width
    .setfixedHeight(100.f) // 100 pixels tall
    .setColor(sf::Color::Red)
    .align(uilo::Align::CENTER_X | uilo::Align::BOTTOM)
    .onLClick([](){ /* callback logic */ });
```

#### Key `Modifier` Methods

  * **Sizing:**
      * `setWidth(float pct)` / `setHeight(float pct)`: Sets size as a percentage (0.0 to 1.0) of the parent's available space.
      * `setfixedWidth(float px)` / `setfixedHeight(float px)`: Sets a fixed size in pixels. **Fixed sizes always take precedence over percentage sizes.**
  * **Alignment:**
      * `align(uilo::Align)`: Controls alignment within the parent container. Flags can be combined using the bitwise OR operator (`|`).
  * **Appearance:**
      * `setColor(sf::Color)`: Sets the background color.
      * `setVisible(bool)`: Shows or hides the element. Hidden elements are excluded from layout calculations.
  * **Event Callbacks:**
      * `onLClick(funcPtr cb)`: Sets a callback function for a left mouse click.
      * `onRClick(funcPtr cb)`: Sets a callback for a right mouse click.
  * **Rendering Order:**
      * `setHighPriority(bool)`: If true, this element and its children will be rendered on top of all other non-high-priority elements. This is essential for dropdowns and popups.

### The `UILO` Class: The Application Core

This class is the main driver of a UILO application. It manages the window, main loop, pages, and global events.

  * **Constructors:**
      * `UILO()`: Creates a default, fullscreen window.
      * `UILO(const std::string& title)`: Creates a default window with a specific title.
      * `UILO(sf::RenderWindow& userWindow, sf::View& view)`: **Integrates UILO into an existing SFML application.** UILO will render to the provided window instead of creating its own.
  * **Main Loop Methods:**
      * `update()`: Polls SFML events, dispatches them to elements, and recalculates the UI layout if changes have occurred.
      * `render()`: Clears the window and draws the current page.
      * `isRunning()`: Returns `true` while the window is open.
  * **Page Management:**
      * `addPage(std::unique_ptr<Page> page, const std::string& name)`: Adds a page to the application.
      * `switchToPage(const std::string& name)`: Sets the active, visible page.

### The `Page` Class: Managing UI Screens

A `Page` is a top-level container that holds the layouts for a specific screen (e.g., "main\_menu", "settings"). Switching between pages allows for managing different UI states.

-----

## Layouts

UILO's layout system is based on nested containers that automatically arrange their children.

### `Column` & `Row`: The Foundation of Layout

These are the two primary layout containers.

  * **`Column`**: Stacks its children vertically. It divides its **height** among its children based on their `Modifier` height settings. Child alignment is controlled by `Align::LEFT`, `Align::CENTER_X`, and `Align::RIGHT`.
  * **`Row`**: Arranges its children horizontally. It divides its **width** among its children based on their `Modifier` width settings. Child alignment is controlled by `Align::TOP`, `Align::CENTER_Y`, and `Align::BOTTOM`.

**Space Division:** When a `Row` or `Column` updates, it first allocates space for all children with **fixed** widths or heights. The remaining space is then divided among the **percentage-based** children according to their specified ratios.

### `ScrollableColumn` & `ScrollableRow`

These containers function identically to their base versions, but if the total size of their children exceeds the container's bounds, the content can be scrolled with the mouse wheel. The content is automatically clipped to the container's view.

  * `setScrollSpeed(float)`: Adjusts the pixel distance per scroll tick.
  * `setOffset(float)`: Programmatically sets the current scroll position.

### `FreeColumn`

This is a specialized container that behaves like a `Column` but **ignores the parent layout flow**. It can be positioned at absolute screen coordinates using `setPosition(sf::Vector2f)`. Its primary use is for creating pop-up elements, such as the options list in a `Dropdown`.

-----

## Widgets

Widgets are the concrete `Element` classes used to build an interface.

### `Text`

Displays a string of text.

```cpp
uilo::text(
    // Modifier for styling
    uilo::Modifier().setfixedHeight(30.f).setColor(sf::Color::Cyan),
    // The string to display
    "Hello, SFML!",
    // Path to a .ttf or .otf font file
    "fonts/arial.ttf",
    // Optional name for later retrieval
    "my_label"
);
```

  * **`setString(const std::string&)`**: Changes the displayed text after creation.

### `Button`

A clickable element that can be rectangular or pill-shaped and can contain text.

```cpp
uilo::button(
    // Modifier for styling and click event
    uilo::Modifier().setfixedSize({150.f, 40.f}).onLClick(myClickFunction),
    // Style: Default, Rect, or Pill
    uilo::ButtonStyle::Pill,
    // Button's label text
    "Confirm",
    // Path to font file
    "fonts/sansation.ttf",
    // Color of the text
    sf::Color::White,
    // Optional name
    "confirm_button"
);
```

  * `isClicked()`: Returns true for the single frame in which the button was clicked.
  * `isHovered()`: Returns true if the mouse cursor is over the button.
  * `setText(const std::string&)`: Changes the button's label.

### `Spacer`

An invisible element used to create empty space within a layout.

```cpp
uilo::column({
    uilo::button(...),
    // Creates a flexible space that pushes the next element down
    uilo::spacer(uilo::Modifier().setHeight(1.0f)),
    uilo::button(...) // This button will be at the bottom
})
```

### `Slider`

A draggable knob on a bar used to select a value within a range (default 0.0 to 1.0).

```cpp
uilo::horizontalSlider(
    // Modifier
    uilo::Modifier().setfixedHeight(20.f).setWidth(0.5f),
    // Knob color
    sf::Color::White,
    // Bar color
    sf::Color(80, 80, 80),
    // Optional name
    "volume_slider"
);
```

  * **Factories:** Use `uilo::verticalSlider(...)` or `uilo::horizontalSlider(...)` for convenience.
  * `getValue()`: Returns the slider's current value.
  * `setValue(float)`: Sets the slider's value.

### `Dropdown`

A button that, when clicked, reveals a vertical list of selectable options.

```cpp
uilo::dropdown(
    // Modifier for the main button
    uilo::Modifier().setfixedSize({200.f, 30.f}),
    // Default text shown before a selection is made
    "Select an Option...",
    // An initializer list of strings for the options
    { "Option A", "Option B", "Option C" },
    // Font path for all text
    "fonts/arial.ttf",
    // Text color
    sf::Color::Black,
    // Background color for the dropdown options
    sf::Color(200, 200, 200),
    // Optional name
    "item_selector"
);
```

  * `getSelected()`: Returns the currently selected `std::string`.
  * **Note:** The dropdown system is designed so that only one dropdown may be open at a time. Clicking outside an open dropdown will close it.

### `Image`

Displays an `sf::Image`.

```cpp
sf::Image myImage;
myImage.loadFromFile("path/to/icon.png");

uilo::image(
    // Modifier to size and position the image
    uilo::Modifier().setfixedSize({64.f, 64.f}),
    // Reference to the sf::Image object
    myImage,
    // (Optional) Recolor non-transparent pixels using the modifier's color
    true,
    // Optional name
    "player_icon"
);
```

  * `setImage(sf::Image&, bool recolor)`: Changes the displayed image after creation.

-----

## Advanced Usage

### Event Handling (`onLClick` / `onRClick`)

Event handling is most commonly achieved by attaching a lambda function directly in an element's `Modifier`.

```cpp
int counter = 0;
uilo::button(
    uilo::Modifier()
        .onLClick([&counter]() { // Use [&] to capture variables by reference
            counter++;
            std::cout << "Counter is now: " << counter << std::endl;
        })
        .onRClick([]() {
            std::cout << "Right click detected!" << std::endl;
        }),
    ...
);
```

### Dynamic UI: Adding and Removing Elements

The UI tree can be modified at runtime.

  * **To Add:** Retrieve a pointer to a container and use its `addElement()` or `addElements()` methods.
  * **To Remove:** Use the `clear()` method on a container. This marks all of its children for deletion and removes them from the container. UILO will deallocate the memory during the next update cycle.

<!-- end list -->

```cpp
// Retrieve a pointer to a column created earlier
uilo::Column* userList = uilo::getColumn("user_list_column");

// Clear any existing users
userList->clear();

// Add new users
for(const auto& userName : newUsers) {
    userList->addElement(uilo::text(Modifier().setfixedHeight(25.f), userName, ...));
}

// Force the UI to recalculate its layout
app.forceUpdate();
```

### Integrating with an Existing SFML Application

UILO can be integrated into an existing SFML application by using a constructor that accepts an `sf::RenderWindow`.

```cpp
#include <SFML/Graphics.hpp>
#include "UILO.hpp"

int main() {
    // Existing SFML window and view
    sf::RenderWindow window(sf::VideoMode(800, 600), "My Game");
    sf::View view = window.getDefaultView();
    window.setFramerateLimit(60);

    // Pass the window and view to UILO
    uilo::UILO ui(window, view);

    // Create pages and elements as normal
    ui.addPage(uilo::page({...}), "hud");
    ui.switchToPage("hud");

    while (window.isOpen()) {
        // Application event loop
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            // Note: UILO::update() handles UI-related events internally
        }

        // 1. Update UILO, passing the application's view
        ui.update(view);

        // 2. Render game world
        window.clear();
        // ... draw game objects ...

        // 3. Render UILO on top
        ui.render();

        window.display();
    }
}
```

### Global Getters for Element Access

If an element is created with a name, a pointer to it can be retrieved from anywhere using a global getter function. This is useful for modifying elements from other parts of an application.

```cpp
// Creation
uilo::text(..., "score_label");
uilo::slider(..., "music_volume");

// Later, in another function...
uilo::Text* scoreLabel = uilo::getText("score_label");
scoreLabel->setString("Score: 100");

uilo::Slider* musicSlider = uilo::getSlider("music_volume");
float volume = musicSlider->getValue();
```

Available getters: `getRow`, `getColumn`, `getSpacer`, `getButton`, `getText`, `getSlider`, `getDropdown`.

-----

## API Reference

### Enums

#### `uilo::Align`

Used with `Modifier::align()` to position an element within its parent container.

  * `NONE`
  * `TOP`
  * `BOTTOM`
  * `LEFT`
  * `RIGHT`
  * `CENTER_X` (Horizontal centering)
  * `CENTER_Y` (Vertical centering)

#### `uilo::ButtonStyle`

Used with the `uilo::button` factory.

  * `Default`: Same as `Rect`.
  * `Pill`: A rectangle with rounded ends.
  * `Rect`: A standard rectangle.

#### `uilo::SliderOrientation`

Used with the `uilo::slider` factory.

  * `Vertical`
  * `Horizontal`

-----
