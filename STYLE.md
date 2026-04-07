# UILO C++ Coding Style Guide

## Indentation & Whitespace

- **4 spaces** for indentation. No tabs.
- No trailing whitespace on any line.
- Files end with a single newline.

## Braces

- **K&R style** — opening brace on the same line as the declaration.
- Applies to classes, structs, functions, `if`/`else`/`for`/`while` blocks.

```cpp
void doSomething() {
    // ...
}

if (condition) {
    // ...
} else {
    // ...
}

class Foo {
};
```

## Namespaces

- **Do not indent** the body of a namespace.
- Close with `} // namespace uilo`.

```cpp
namespace uilo {

class MyClass {
    // ...
};

} // namespace uilo
```

## Naming Conventions

| Entity             | Convention                  | Example                |
|--------------------|-----------------------------|------------------------|
| Classes / Structs  | PascalCase                  | `Element`, `Modifier`  |
| Methods / Functions| camelCase                   | `getBounds()`, `resize()` |
| Member Variables   | `m_` prefix + camelCase     | `m_bounds`, `m_renderScale` |
| Local Variables    | camelCase                   | `mousePosition`, `dt`  |
| Enum Classes       | PascalCase type, UPPER_CASE values | `Align::CENTER_X` |
| Namespaces         | lowercase                   | `uilo`, `Colors`       |
| Type Aliases       | PascalCase                  | `FuncPtr`, `ScrollFuncPtr` |

## Pointer & Reference Alignment

Attach `*` and `&` to the **type**, not the variable name:

```cpp
Element* element;
const Bounds& parent;
Modifier& getModifier();
```

## Include Guards

Use `#pragma once` exclusively. No `#ifndef` guards.

## Access Specifiers

- Not indented (flush with the `class` keyword).
- One blank line before each access specifier **except** the first one in a class.
- Remove empty `public:` / `protected:` / `private:` sections that contain no members.

```cpp
class Element {
public:
    void update();

private:
    int m_value;
};
```

## Constructor Initializer Lists

- Colon on the **next line**, indented 4 spaces.
- For short single-initializer constructors, keeping it on the same line is acceptable.
- Opening brace on its own line when using a multi-line initializer list, or on the same line for compact constructors.

```cpp
// Multi-line initializer list
Text::Text(Modifier modifier, uint16_t fontSize, const std::string& name)
    : m_fontSize(fontSize), m_string(name)
{
    m_modifier = modifier;
}

// Short initializer — same line OK
UILO::UILO()
    : m_lastTime(std::chrono::steady_clock::now())
{}

// Single-init compact form
explicit SFMLRenderer(sf::RenderWindow& window) : m_window(window) {
    // ...
}
```

## Inline Methods

- **Trivial getters/setters** (single expression, no control flow) may be on one line:

```cpp
float getScale() const { return m_scale; }
void setVisible(bool visible) { m_visible = visible; }
```

- Methods with **control flow or multiple statements** must use multi-line formatting:

```cpp
void drawFilledRect(Rect& rect) {
    if (!m_drawFilledRect) return;
    if (m_renderScale == 1.f) {
        m_drawFilledRect(&rect);
        return;
    }
    Rect scaled;
    applyScale(rect, scaled);
    m_drawFilledRect(&scaled);
}
```

## Comments

- Use `//` line comments exclusively. No `/* */` block comments.
- Multi-line documentation uses multiple `//` lines:

```cpp
// StateMachine
// Usage: StateMachine sm;
//        sm.addState(idleState);
```

## Blank Lines

- One blank line between method definitions in `.cpp` files.
- One blank line between logical groups in headers (e.g., between setter block and getter block).
- No multiple consecutive blank lines.

## Member Variable Alignment

- Use simple spacing — no columnar padding across unrelated declarations:

```cpp
float m_renderScale = 1.f;
std::string m_name = "";
bool m_visible = true;
```

## `explicit` Keyword

Use `explicit` on constructors that could be called with a single argument to prevent implicit conversions:

```cpp
explicit SFMLRenderer(sf::RenderWindow& window);
explicit SDLRenderer(SDL_Window* window, SDL_GLContext ctx);
```

## Float Literals

Use the `f` suffix for float literals: `0.f`, `1.f`, `0.5f`, `3.14159265f`.

## Spacing

- Space after keywords: `if (`, `for (`, `while (`, `return `.
- Space after commas in parameter lists: `func(a, b, c)`.
- Spaces around binary operators: `x + y`, `a == b`.
- No space before semicolons.
- No space between function name and opening paren: `doSomething()`.
