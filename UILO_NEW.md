# UILO_NEW — C++17 / SFML 3 UI Library

## Project Root
`/home/harrison/Dev/C++/UILO_NEW/`

## Build System
- `make lib` — builds `lib/libuilo.a` from `obj/` (mirrors `include/` structure)
- `make clean && make lib` — required after any header change (Makefile lacks transitive header deps)
- `./build.sh [--clean]` — root build script
- `examples/build.sh [--clean] [name|all]` — builds examples; auto-detects .cpp files
- CXXFLAGS: `-std=c++17 -Wall -Wextra -I include -I include/elements`
- SFML 3.1.0 at `/usr/lib/libsfml-*.so.3.1.0`; link with `-lsfml-graphics -lsfml-window -lsfml-system`

## SFML 3 Key API Differences (from SFML 2)
- `sf::Font::openFromFile()` (not `loadFromFile()`)
- `sf::Sprite(const Texture&)` — no default ctor; use `std::optional<sf::Sprite>`
- `sf::Text(const Font&, string, unsigned int)` — no default ctor; use `std::optional<sf::Text>`
- `sf::Texture::loadFromImage()` is `[[nodiscard]] bool`
- `sf::RenderTexture::resize(sf::Vector2u)` is `[[nodiscard]] bool`; default ctor exists
- `sf::Text::LineAlignment` enum: `Default`, `Left`, `Center`, `Right`
- `sf::Text::Style::Bold/Italic/Underlined/StrikeThrough`
- `sf::PrimitiveType::TriangleFan` exists
- `sf::BlendMode::Factor` and `sf::BlendMode::Equation` are scoped enums
- `sf::RenderTarget` is base for both `sf::RenderWindow` and `sf::RenderTexture`
- All `render()` signatures use `sf::RenderTarget&` (not `sf::RenderWindow&`)

## Source Structure
```
include/
  UILO.hpp / UILO.cpp          — main class; owns element pool, pages, window ptr
  Page.hpp / Page.cpp          — wraps root Container; render(sf::RenderTarget&)
  Elements.hpp                 — master include for all elements + Factory.hpp
  elements/
    Element.hpp / Element.cpp  — base class; virtual render(sf::RenderTarget&), update()
    Modifier.hpp / Modifier.cpp — config chain (fluent); ALL element signatures take Modifier first
    Factory.hpp                — inline factory functions (column, row, spacer, image, text, button, page)
    containers/
      Container.hpp / Container.cpp — base container; m_children, m_rt (sf::RenderTexture), pruneChildren()
      Column.hpp / Column.cpp  — vertical layout
      Row.hpp / Row.cpp        — horizontal layout
    decoration/
      Spacer.hpp / Spacer.cpp
      Image.hpp / Image.cpp
      Text.hpp / Text.cpp
    interactible/
      Button.hpp / Button.cpp
  utils/
    Alignment.hpp  — Align enum (Top/Bottom/Left/Right/CenterX/CenterY), hasAlign()
    Dimension.hpp  — Dimension struct + _px / _pct literals
    Timer.hpp
    Utils.hpp      — includes Alignment, Dimension, Timer
    RenderUtils.hpp — makeRoundedRect(), eraseCorners() (pure SFML, no shaders)
```

## Modifier (fluent chain)
```cpp
Modifier()
  .setWidth(Dimension)        // default 100_pct
  .setHeight(Dimension)       // default 100_pct
  .setColor(sf::Color)        // default White
  .setAlign(Align)            // default Left|Top
  .setOuterPadding(float)
  .setRounding(float)         // corner radius; 0 = no rounding
  .setFreePosition(sf::Vector2f)
  .setVisible(bool)
  .setOnLeftClick(FuncPtr)
  .setOnRightClick(FuncPtr)
  .setOnHover(FuncPtr, float delay=0)
  .setOnScroll(ScrollFuncPtr)
```

## ImageOptions (bitmask enum, uint8_t)
`NONE, LockAspectWidth, LockAspectHeight, Recolor, ClipEllipse, FlipH, FlipV`
- Recolor: luminance-based pixel replacement using modifier color
- ClipEllipse: pixel-level ellipse mask
- Operator `|` defined

## TextOptions (bitmask enum, uint16_t)
`NONE, LeftAlign, RightAlign, CenterX, CenterY, Wrap, Bold, Italic, Underlined, StrikeThrough, TopAlign, BottomAlign`
- Operator `|` defined
- Wrap triggers word-wrap rebuild when bounds width changes
- Y priority: CenterY > BottomAlign > top (default/TopAlign)

## Corner Rounding (include/utils/RenderUtils.hpp)
- `makeRoundedRect(pos, size, r, segs=8)` → `sf::ConvexShape` (used by Spacer)
- `eraseCorners(rt, bounds, r, segs=8)` — draws 4 TriangleFan shapes with erase blend
  - Erase blend: `BlendMode(Zero, One, Add, Zero, Zero, Add)` — keeps colour, zeroes alpha
- Column/Row with rounding > 0: render into window-sized `m_rt`, call eraseCorners, composite sprite back
- Performance note: each rounded container = 1 full RT clear + 1 display() stall per frame

## Element Ownership
- UILO owns all elements via `m_elementPool` (`vector<unique_ptr<Element>>`)
- `Element::setUILO()` adds `this` to pool; `Container::setUILO()` propagates to children
- `Button` pushes its `Text*` to `m_children`; setUILO propagation adds it to the pool automatically
- Factory functions return raw `new`-allocated pointers; UILO takes ownership via setUILO

## Button
- Inherits `Row` (gets update/render/rounding/clipping for free)
- Constructor: `Button(Modifier, Text*, name="")`
- Overrides `checkLeft/RightClick`, `checkHover`, `checkScroll` to use `Element::check*` directly
  (bypasses Container child propagation so clicks always fire button's own handler)
- Factory: `button(Modifier, Text*, name="")`

## UILO Usage Pattern
```cpp
UILO ui(window, page(
    column(Modifier().setColor(bg), {
        row(Modifier().setHeight(64_px).setRounding(12.f), {}),
        button(Modifier().setRounding(8.f).setOnLeftClick([]{}),
               text(Modifier().setAlign(Align::CenterX|Align::CenterY).setColor(sf::Color::White),
                    "assets/fonts/Font.ttf", "Click", 24, TextOptions::CenterX|TextOptions::CenterY))
    }, "root"), "main_page"
));
// in loop:
ui.update();
window.clear();
ui.render();
window.display();
```

## Known Issues / Conventions
- `make clean && make lib` needed after header changes — Makefile doesn't track transitive header deps
- Spacer has a pre-existing `-Wunused-parameter` warning on `dt` in `update()` — benign
- Examples run from `examples/` dir (assets are relative paths like `"assets/fonts/..."`)
- `MIDX`/`MIDY` are old aliases for `CenterX`/`CenterY` in Align (check Alignment.hpp for current names)
