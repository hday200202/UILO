#pragma once

#include "elements/Elements.hpp"
#include "../Page.hpp"

namespace uilo {

inline Column* column(
    Modifier modifier = {}, 
    ColumnOptions options = {}, 
    contains children = {}, 
    const std::string& name = ""
) { return new Column(modifier, options, children, name); }

inline Row* row(
    Modifier modifier = {}, 
    RowOptions options = {}, 
    contains children = {}, 
    const std::string& name = ""
) { return new Row(modifier, options, children, name); }

inline Canvas* canvas(
    Modifier modifier = {},
    CanvasOptions options = {},
    contains children = {},
    const std::string& name = ""
) { return new Canvas(modifier, options, children, name); }

// freeColumn / freeRow build a normal Column/Row but mark the result as
// floating: it lives outside the page layout flow and is positioned in
// window space by UILO::addFloating(). Width and height come from the
// Modifier (use the same _px / _pct sugar as regular layout); position
// is set via .setPosition(Dimension x, Dimension y) on the FreeElement.
struct FreeElement {
    Element*  element   = nullptr;
    Dimension xPos      = 0_px;
    Dimension yPos      = 0_px;
    bool      draggable = false;

    FreeElement& setPosition(Dimension x, Dimension y) { xPos = x; yPos = y; return *this; }
    FreeElement& setDraggable(bool d) { draggable = d; return *this; }
};

inline FreeElement freeColumn(
    Modifier modifier = {},
    ColumnOptions options = {},
    contains children = {},
    const std::string& name = ""
) { return { new Column(modifier, options, children, name) }; }

inline FreeElement freeRow(
    Modifier modifier = {},
    RowOptions options = {},
    contains children = {},
    const std::string& name = ""
) { return { new Row(modifier, options, children, name) }; }

inline Spacer* spacer(
    Modifier modifier = {}, 
    SpacerOptions options = {}, 
    const std::string& name = ""
) { return new Spacer(modifier, options, name); }

inline Image* image(
    Modifier modifier = {}, 
    ImageOptions options = {}, 
    const std::string& name = ""
) { return new Image(modifier, options, name); }

inline Text* text(
    Modifier modifier = {}, 
    TextOptions options = {}, 
    const std::string& name = ""
) { return new Text(modifier, options, name); }

inline Waveform* waveform(
    Modifier modifier = {},
    WaveformOptions options = {},
    const std::string& name = ""
) { return new Waveform(modifier, options, name); }

inline Page* page(
    Container* root, 
    const std::string& name
) { return new Page(root, name); }

inline Button* button(
    Modifier modifier = {}, 
    ButtonOptions options = {}, 
    const std::string& name = ""
) { return new Button(modifier, options, name); }

inline Slider* slider(
    Modifier modifier = {}, 
    SliderOptions options = {}, 
    const std::string& name = ""
) { return new Slider(modifier, options, name); }

inline Knob* knob(
    Modifier modifier = {},
    KnobOptions options = {},
    const std::string& name = ""
) { return new Knob(modifier, options, name); }

inline Dropdown* dropdown(
    Modifier modifier = {}, 
    DropdownOptions options = {},
    std::initializer_list<std::string> items = {},
    const std::string& name = ""
) { return new Dropdown(modifier, options, items, name); }

inline Resizer* resizer(
    Modifier modifier = {}, 
    ResizerOptions options = {}, 
    const std::string& name = ""
) { return new Resizer(modifier, options, name); }

inline Textbox* textbox(
    Modifier modifier = {}, 
    TextboxOptions options = {}, 
    const std::string& name = ""
) { return new Textbox(modifier, options, name); }

}