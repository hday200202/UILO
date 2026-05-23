#pragma once

#include "elements/Elements.hpp"
#include "../Page.hpp"

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

inline Image* image(
    Modifier modifier,
    const std::string& path,
    ImageOptions options = ImageOptions::NONE,
    const std::string& name = ""
) {
    return new Image(modifier, path, options, name);
}

inline Image* image(
    Modifier modifier,
    sf::Image sourceImg,
    ImageOptions options = ImageOptions::NONE,
    const std::string& name = ""
) {
    return new Image(modifier, std::move(sourceImg), options, name);
}

inline Text* text(
    Modifier modifier,
    const std::string& fontPath,
    const std::string& content,
    unsigned int charSize,
    TextOptions options = TextOptions::NONE,
    const std::string& name = ""
) {
    return new Text(modifier, fontPath, content, charSize, options, name);
}

inline Text* text(Modifier modifier,
    const sf::Font& font,
    const std::string& content,
    unsigned int charSize,
    TextOptions options = TextOptions::NONE,
    const std::string& name = ""
) {
    return new Text(modifier, font, content, charSize, options, name);
}

inline Page* page(Container* root, const std::string& name) {
    return new Page(root, name);
}

inline Button* button(Modifier modifier, Text* text, const std::string& name = "") {
    return new Button(modifier, text, name);
}

}