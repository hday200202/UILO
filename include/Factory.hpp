#pragma once

#include "elements/Elements.hpp"
#include "Page.hpp"

namespace uilo {

inline Column* column(Modifier modifier, contains children, const std::string& name = "") {
    return new Column(modifier, children, name);
}

inline Row* row(Modifier modifier, contains children, const std::string& name = "") {
    return new Row(modifier, children, name);
}

inline Spacer* spacer(Modifier modifier, const std::string& name = "") {
    return new Spacer(modifier, name);
}

inline Text* text(Modifier modifier, uint16_t fontSize, const std::string& string, const std::string& fontName = "", bool wordWrap = false, const std::string& name = "") {
    return new Text(modifier, fontSize, string, fontName, wordWrap, name);
}

inline Image* image(Modifier modifier, const uint8_t* pixels, uint32_t width, uint32_t height, bool keepAspectRatio = true, const std::string& name = "") {
    return new Image(modifier, pixels, width, height, keepAspectRatio, name);
}

inline Page* page(Container* root, const std::string& name) {
    return new Page(root, name);
}

} // namespace uilo
