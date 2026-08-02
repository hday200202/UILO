# Translator.cpp

`include/wt/Translator.cpp`

[← index](../README.md)

## Types

- [KnobWidget](#knobwidget)
- [KnobGeometry](#knobgeometry)
- [kTextboxJs](#ktextboxjs)

## Functions

- [`num(float v)`](#num)
- [`px(float v)  { return num(v)`](#px)
- [`pct(float v) { return num(v)`](#pct)
- [`rgba(Color c)`](#rgba)
- [`rgbaWith(Color c, float alpha)`](#rgbawith)
- [`crossAlign(Align a, Axis parentAxis)`](#crossalign)
- [`bucketOf(Align a, Axis axis)`](#bucketof)
- [`textJustify(Align a)`](#textjustify)
- [`textAlign(Align a)`](#textalign)
- [`textAlignItems(Align a)`](#textalignitems)
- [`isRowLike(ElementType t)`](#isrowlike)
- [`axisOf(Element* el)`](#axisof)
- [`backgroundOf(Element* el, Background& out)`](#backgroundof)
- [`aspectLocked(const ImageOptions& o)`](#aspectlocked)
- [`knobGeometry(Knob* k)`](#knobgeometry)
- [`imageSize(const std::string& path, uint32_t& w, uint32_t& h)`](#imagesize)
- [`flipCss(const ImageOptions& o)`](#flipcss)
- [`isScrollable(Element* el)`](#isscrollable)
- [`textboxLineHeight(const TextboxOptions& o)`](#textboxlineheight)
- [`sliderSteps(const SliderOptions& o)`](#slidersteps)
- [`gradientCss(const Color c[4])`](#gradientcss)
- [`appendMaterial(std::string& css, const Material& mat, Color elementColor)`](#appendmaterial)
- [`Translator(Wt::WApplication& app, UILO& uilo, const Config& config)`](#translator)
- [`styleKey(const std::string& css, const PseudoRules& pseudo)`](#stylekey)
- [`addVendorRule(const std::string& rule)`](#addvendorrule)
- [`classFor(const std::string& css, const PseudoRules& pseudo)`](#classfor)
- [`fontFamilyFor(const std::string& path)`](#fontfamilyfor)
- [`jsDimension(Dimension d)`](#jsdimension)
- [`ownHeightExpr(...)`](#ownheightexpr)
- [`styleFor(const Node& n, PseudoRules& pseudo)`](#stylefor)
- [`applyImageAspect(Image* img)`](#applyimageaspect)
- [`buildKnob(Knob* k, KnobWidget* w)`](#buildknob)
- [`buildResizer(Resizer* r, Wt::WContainerWidget* wrapper, Axis axis)`](#buildresizer)
- [`apply(Node& n)`](#apply)
- [`sync()`](#sync)
- [`syncFloating(const std::vector&lt;Element*&gt;& floating)`](#syncfloating)
- [`buildOverlay(Element* backdrop)`](#buildoverlay)
- [`translate(Element* el, Wt::WContainerWidget* parent, Node node)`](#translate)
- [`translateChildren(...)`](#translatechildren)
- [`build(Page& page, Wt::WContainerWidget* into)`](#build)

---

### KnobWidget

A plain box that also owns the JSignal the drag script hands the settled value to. A knob is drawn entirely in CSS, so the only reason this is a subclass at all is that a JSignal has to live on a WObject.

---

### KnobGeometry

The knob's arc laid out the way CSS wants it. [UILO](../UILO.hpp.md#uilo) measures angles from +x with y running down; a conic-gradient starts at 12 o'clock and sweeps clockwise, which is the same direction -- just rotated a quarter turn, hence the +90 everywhere. `span` is always positive because a gradient's stops must ascend, so a clockwise knob starts from the far end and fills backwards (`rev`).

---

### kTextboxJs

Client-side support for the two [Textbox](../elements/interactible/Textbox.hpp.md#textbox) features the browser has no equivalent of: a Tab key that indents instead of moving focus, and a line-number gutter beside a multiline box.

> The gutter is built entirely here rather than as Wt widgets, so it never enters the widget tree the [Translator](Translator.hpp.md#translator) syncs -- nothing it does can disturb the flex layout, and if any step fails the box simply renders without a gutter. It is positioned over the textarea's own left padding, which the script widens to make room, so the two can never disagree about the width.

> Soft-wrapped lines are measured with a hidden mirror element that copies the textarea's font and wrap width, so one logical line gets one number however many visual rows it occupies. That is what makes the web gutter agree with the native one, which numbers logical lines too.

---

### num

```cpp
num(float v)
```

**Parameters**

- `float v`

**Returns** — std::string

------------------------------------------------------------ --------------- CSS value formatting ----------------------- ----------------------------------------------------

---

### px

```cpp
px(float v)  { return num(v)
```

**Parameters**

- `float v)  { return num(v`

**Returns** — std::string

Formats a number as a CSS pixel length.

---

### pct

```cpp
pct(float v) { return num(v)
```

**Parameters**

- `float v) { return num(v`

**Returns** — std::string

Formats a number as a CSS percentage.

---

### rgba

```cpp
rgba(Color c)
```

**Parameters**

- `Color c`

**Returns** — std::string

Formats a [Color](../utils/Color.hpp.md#color) as a CSS rgba() literal, so alpha survives the trip.

---

### rgbaWith

```cpp
rgbaWith(Color c, float alpha)
```

**Parameters**

- `Color c`
- `float alpha`

**Returns** — std::string

Same colour, explicit alpha. Materials need this: the shader's body opacity is separate from the tint's own alpha.

---

### crossAlign

```cpp
crossAlign(Align a, Axis parentAxis)
```

**Parameters**

- `Align a`
- `Axis parentAxis`

**Returns** — const char*

Cross-axis placement, in the order [Element](../elements/Element.hpp.md#element)::resize() tests the flags: the edge flags win over the centre flag when both are set.

---

### bucketOf

```cpp
bucketOf(Align a, Axis axis)
```

**Parameters**

- `Align a`
- `Axis axis`

**Returns** — int

Which of a container's three layout groups a child belongs to: 0 = start, 1 = centre, 2 = end. Mirrors the bucket test in [Row](../elements/containers/Row.hpp.md#row)/[Column](../elements/containers/Column.hpp.md#column)::update().

---

### textJustify

```cpp
textJustify(Align a)
```

**Parameters**

- `Align a`

**Returns** — const char*

[Text](../elements/decoration/Text.hpp.md#text)'s own alignment is a single enum value, not a flag set -- [Text](../elements/decoration/Text.hpp.md#text)::render() switches on it exactly -- so these compare rather than test bits.

---

### textAlign

```cpp
textAlign(Align a)
```

**Parameters**

- `Align a`

**Returns** — const char*

Maps [UILO](../UILO.hpp.md#uilo)'s horizontal [Align](../utils/Alignment.hpp.md#align) onto the CSS text-align keyword.

---

### textAlignItems

```cpp
textAlignItems(Align a)
```

**Parameters**

- `Align a`

**Returns** — const char*

Maps [UILO](../UILO.hpp.md#uilo)'s cross-axis [Align](../utils/Alignment.hpp.md#align) onto the CSS align-items keyword, which is how a flex child is placed across the axis its parent runs on.

---

### isRowLike

```cpp
isRowLike(ElementType t)
```

**Parameters**

- `ElementType t`

**Returns** — bool

------------------------------------------------------------ --------------- [Element](../elements/Element.hpp.md#element) introspection ---------------------- -----------------------------------------------------

---

### axisOf

```cpp
axisOf(Element* el)
```

**Parameters**

- `Element* el`

**Returns** — Axis

Which axis a container lays out on, so its children can be emitted as flex items in the matching direction.

---

### backgroundOf

```cpp
backgroundOf(Element* el, Background& out)
```

**Parameters**

- `Element* el`
- `Background& out`

**Returns** — bool

Reads whichever background an element carries -- flat colour, gradient or material -- into one struct, so the CSS is emitted from a single shape instead of branching per element type.

---

### aspectLocked

```cpp
aspectLocked(const ImageOptions& o)
```

**Parameters**

- `const ImageOptions& o`

**Returns** — bool

With either lock set, [Image](../elements/decoration/Image.hpp.md#image)::update() overwrites one axis of its bounds with (other axis / aspect), so the drawn box has the picture's own proportions no matter what size the layout hands it.

---

### knobGeometry

```cpp
knobGeometry(Knob* k)
```

**Parameters**

- `Knob* k`

**Returns** — [KnobGeometry](#knobgeometry)

The knob's arc geometry, computed through the same sweepDegrees and angleForValue the native renderer uses, so the browser draws the same arc rather than an approximation of it.

---

### imageSize

```cpp
imageSize(const std::string& path, uint32_t& w, uint32_t& h)
```

**Parameters**

- `const std::string& path`
- `uint32_t& w`
- `uint32_t& h`

**Returns** — bool

Reads just the dimensions out of an image file's header. [Image](../elements/decoration/Image.hpp.md#image)::init() gets these from the decoded texture; the web backend has no decoder, but the header is all that's needed and every one of these formats puts it up front.

---

### flipCss

```cpp
flipCss(const ImageOptions& o)
```

**Parameters**

- `const ImageOptions& o`

**Returns** — std::string

Mirror-image transform for a flipped [Image](../elements/decoration/Image.hpp.md#image), or empty.

---

### isScrollable

```cpp
isScrollable(Element* el)
```

**Parameters**

- `Element* el`

**Returns** — bool

Whether a container scrolls, which decides between an overflow rule and letting the content size the box.

---

### textboxLineHeight

```cpp
textboxLineHeight(const TextboxOptions& o)
```

**Parameters**

- `const TextboxOptions& o`

**Returns** — float

[Textbox](../elements/interactible/Textbox.hpp.md#textbox)::lineHeight() is charSize * 1.2, so CSS can match it exactly. It has to be a definite number rather than `normal`: the auto-grow script converts a maxResizeLines cap into pixels and must agree with the rendered leading.

---

### sliderSteps

```cpp
sliderSteps(const SliderOptions& o)
```

**Parameters**

- `const SliderOptions& o`

**Returns** — int

A Wt slider is integral where [UILO](../UILO.hpp.md#uilo)'s is float, so the float range is carried as a whole number of steps. Using the declared step means one arrow-key press moves the slider exactly one step, the way it does natively; a continuous slider has no such anchor and gets a fixed resolution instead.

---

### gradientCss

```cpp
gradientCss(const Color c[4])
```

**Parameters**

- `const Color c[4]`

**Returns** — std::string

[UILO](../UILO.hpp.md#uilo) interpolates four corner colours bilinearly on the GPU. CSS has no equivalent, but the two cases the [Gradient](../utils/Gradient.hpp.md#gradient) API actually encourages -- a vertical or horizontal fade, via setTop/setBottom or setLeft/setRight -- are plain linear- gradients. Anything genuinely four-cornered falls back to a diagonal through the two opposite corners.

---

### appendMaterial

```cpp
appendMaterial(std::string& css, const Material& mat, Color elementColor)
```

**Parameters**

- `std::string& css`
- `const Material& mat`
- `Color elementColor`

Materials are shader effects natively. The browser's nearest equivalent is a backdrop-filter, which covers the frosted- pane look the static kinds are after. The animated kinds (Shimmer, Aurora, Holographic, Liquid) keep their tint and blur but lose the animation.

---

### Translator

```cpp
Translator(Wt::WApplication& app, UILO& uilo, const Config& config)
```

**Parameters**

- `Wt::WApplication& app`
- `UILO& uilo`
- `const Config& config`

**Returns** — T

------------------------------------------------------------ --------------- [Translator](Translator.hpp.md#translator) --------------------------------- ------------------------------------------

---

### styleKey

```cpp
styleKey(const std::string& css, const PseudoRules& pseudo)
```

**Parameters**

- `const std::string& css`
- `const PseudoRules& pseudo`

**Returns** — std::string

Identity of a style: the declarations plus every pseudo rule hanging off it, so two elements share a class only when both match.

---

### addVendorRule

```cpp
addVendorRule(const std::string& rule)
```

**Parameters**

- `const std::string& rule`

Wt adds rules with CSSStyleSheet.insertRule, which *throws* on a selector the browser does not recognise -- and each engine only recognises its own vendor-prefixed pseudo- elements. One `::-moz-range-track` is therefore enough to abort the whole stylesheet in Chrome and leave the page blank. Appending text to a <style> element goes through the ordinary CSS parser instead, which drops rules it cannot parse and keeps the rest, which is the behaviour these rules were written expecting.

---

### classFor

```cpp
classFor(const std::string& css, const PseudoRules& pseudo)
```

**Parameters**

- `const std::string& css`
- `const PseudoRules& pseudo`

**Returns** — std::string

Interns a CSS rule and returns the class name for it, so identical styling across many elements produces one rule rather than one per widget. Pseudo-class rules are interned with it, since they belong to the same selector.

---

### fontFamilyFor

```cpp
fontFamilyFor(const std::string& path)
```

**Parameters**

- `const std::string& path`

**Returns** — std::string

Turns a font path into a CSS font-family, registering an @font- face for it the first time it is seen so the browser fetches the same file the native build loads.

---

### jsDimension

```cpp
jsDimension(Dimension d)
```

**Parameters**

- `Dimension d`

**Returns** — std::string

A [Dimension](../utils/Dimension.hpp.md#dimension) as the {v, pct} pair the script above expects.

---

### ownHeightExpr

```cpp
ownHeightExpr(...)
```

**Parameters**

- `Element* el`
- `Axis parentAxis`
- `bool parentScrolls`
- `const std::string& parentHeight`

**Returns** — std::string

The element's drawn height as a CSS length, when the tree alone determines it. Used to size text that has no explicit charSize, which [UILO](../UILO.hpp.md#uilo) derives from the box height. Empty means "only the browser knows", and callers fall back.

---

### styleFor

```cpp
styleFor(const Node& n, PseudoRules& pseudo)
```

**Parameters**

- `const Node& n`
- `PseudoRules& pseudo`

**Returns** — std::string

Builds the whole CSS rule for one element: box, spacing, placement, background, and whatever its own type adds. This is the single place a [UILO](../UILO.hpp.md#uilo) element becomes CSS, so the two backends cannot drift on a property without it showing up here.

---

### applyImageAspect

```cpp
applyImageAspect(Image* img)
```

**Parameters**

- `Image* img`

Applies an [Image](../elements/decoration/Image.hpp.md#image)'s aspect lock as a CSS aspect-ratio, so the browser sizes the element the way [UILO](../UILO.hpp.md#uilo)'s own layout would.

---

### buildKnob

```cpp
buildKnob(Knob* k, KnobWidget* w)
```

**Parameters**

- `Knob* k`
- `KnobWidget* w`

Builds the knob's widget tree and the client-side script that repaints its arc while dragging, so the control responds without a round trip to the server.

---

### buildResizer

```cpp
buildResizer(Resizer* r, Wt::WContainerWidget* wrapper, Axis axis)
```

**Parameters**

- `Resizer* r`
- `Wt::WContainerWidget* wrapper`
- `Axis axis`

Builds a resizer as an absolutely-positioned strip straddling the boundary between its siblings, plus the client-side drag script. Positioned rather than laid out, so like the native one it takes no space from the flow.

---

### apply

```cpp
apply(Node& n)
```

**Parameters**

- `Node& n`

Re-applies the properties of one already-built node: its style class, and the live value of a [Text](../elements/decoration/Text.hpp.md#text), [Slider](../elements/interactible/Slider.hpp.md#slider), [Dropdown](../elements/interactible/Dropdown.hpp.md#dropdown) or [Textbox](../elements/interactible/Textbox.hpp.md#textbox). This runs every sync, so it is deliberately cheap and does nothing structural.

---

### sync

```cpp
sync()
```

Reflect any popup opened or closed since the last sync before restyling, so the overlay's widgets exist to receive it.

---

### syncFloating

```cpp
syncFloating(const std::vector<Element*>& floating)
```

**Parameters**

- `const std::vector<Element*>& floating`

Toggle the overlays we already have: up if their backdrop is still floating, hidden otherwise. This is what closes a popup -- open() adds the backdrop, close() removes it, and the state falls out of the list.

---

### buildOverlay

```cpp
buildOverlay(Element* backdrop)
```

**Parameters**

- `Element* backdrop`

Builds an on-top overlay for a floating backdrop the first time it is seen. A picker keeps one backdrop across open and close, so it is translated once and thereafter only shown or hidden.

---

### translate

```cpp
translate(Element* el, Wt::WContainerWidget* parent, Node node)
```

**Parameters**

- `Element* el`
- `Wt::WContainerWidget* parent`
- `Node node`

Creates the Wt widget for one element and recurses into its children. Dispatches on element type, so each one maps to the closest native HTML control -- a [Dropdown](../elements/interactible/Dropdown.hpp.md#dropdown) becomes a select, a [Textbox](../elements/interactible/Textbox.hpp.md#textbox) an input or textarea -- rather than being reimplemented in divs.

---

### translateChildren

```cpp
translateChildren(...)
```

**Parameters**

- `Container* container`
- `Wt::WContainerWidget* parent`
- `Axis axis`
- `bool scrolls`
- `const std::string& parentHeight`

[UILO](../UILO.hpp.md#uilo) lays a container out in three groups -- start, centre, end -- and draws them in that order regardless of child order, so the widgets are emitted grouped the same way.

---

### build

```cpp
build(Page& page, Wt::WContainerWidget* into)
```

**Parameters**

- `Page& page`
- `Wt::WContainerWidget* into`

Walks a page once and creates a widget for every element in it. Called once per session; everything afterwards goes through sync().
