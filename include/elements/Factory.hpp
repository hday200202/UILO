#pragma once

#include "elements/Elements.hpp"
#include "../Page.hpp"

namespace uilo {

inline Column* column(Modifier modifier, ColumnOptions options, contains children, const std::string& name = "") {
    return new Column(modifier, options, children, name);
}

inline Row* row(Modifier modifier, RowOptions options, contains children, const std::string& name = "") {
    return new Row(modifier, options, children, name);
}

inline Spacer* spacer(Modifier modifier, SpacerOptions options = {}, const std::string& name = "") {
    return new Spacer(modifier, options, name);
}

inline Image* image(Modifier modifier, ImageOptions options = {}, const std::string& name = "") {
    return new Image(modifier, options, name);
}

inline Text* text(Modifier modifier, TextOptions options = {}, const std::string& name = "") {
    return new Text(modifier, options, name);
}

inline Page* page(Container* root, const std::string& name) {
    return new Page(root, name);
}

inline Button* button(Modifier modifier, ButtonOptions options = {}, const std::string& name = "") {
    return new Button(modifier, options, name);
}

inline Slider* slider(Modifier modifier, SliderOptions options = {}, const std::string& name = "") {
    return new Slider(modifier, options, name);
}

inline Dropdown* dropdown(Modifier modifier, DropdownOptions options,
                          std::initializer_list<std::string> items,
                          const std::string& name = "") {
    return new Dropdown(modifier, options, items, name);
}

inline Resizer* resizer(Modifier modifier = {}, ResizerOptions options = {}, const std::string& name = "") {
    return new Resizer(modifier, options, name);
}

}