/* ---------------------------------------------------------------------------- //
  Uilo - Retained-mode UI library for SFML
  Author: Harrison Day 2025
  License: MPL-2.0 (see LICENSE.md for full terms)

  This software is provided as-is. Redistribution and modification are allowed
  under the terms of the Mozilla Public License 2.0.
// ---------------------------------------------------------------------------- */

#ifndef UILO_HPP
#define UILO_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

namespace uilo {

enum class Align : uint8_t {
    NONE        = 0,
    TOP         = 1 << 0,
    BOTTOM      = 1 << 1,
    LEFT        = 1 << 2,
    RIGHT       = 1 << 3,
    CENTER_H    = 1 << 4,
    CENTER_V    = 1 << 5
};

// ---------------------------------------------------------------------------- BASE
class Element {
public:
    Element(){}
    virtual ~Element() = default;

    virtual void update() {}
    virtual void render(sf::RenderTarget& target) const {}
    
    Element* onClick(std::function<void()>& func)        { return this; }
    Element* onHover(std::function<void()>& func)        { return this; }
    Element* onRelease(std::function<void()>& func)      { return this; }

    Element* setPosition(float percentage)              { return this; }
    Element* setFixedWidth(float width)                 { return this; }
    Element* setFixedHeight(float height)               { return this; }
    Element* setFixedSize(float width, float height)    { return this; }

protected:
    float m_aspectRatio = 1.f;

    Align m_align = Align::NONE;

    sf::Vector2f m_position;
    sf::Vector2f m_size;

    std::function<void()> m_onClick = nullptr;
    std::function<void()> m_onHover = nullptr;
    std::function<void()> m_onRelease = nullptr;

    bool m_fixedWidth = false;
    bool m_fixedHeight = false;
    bool m_fixedSize = false;

private:

};

// ---------------------------------------------------------------------------- DESIGN / LAYOUT
class Text : public Element {
public:
    Text(const std::string& str = "", const std::string& fontPath = "", float size = 0.f, Align align = Align::NONE) {}
    ~Text() {}

    void update() {}
    void render(sf::RenderTarget& target) {}

private:
    sf::Font m_font;
    sf::Text m_text = sf::Text(m_font);
};

class Spacer : public Element {
public:

private:
    
};

class Image : public Element {

};

// ---------------------------------------------------------------------------- CONTROLS
class Control : public Element {

};

class TextField : public Control {

};

class Button : public Control {

};

// ---------------------------------------------------------------------------- CONTAINERS
class Container : public Element {
public:

private:
    std::vector<Element*> m_elements;
};

class Row : public Container {
public:

private:

};

class Column : public Container {
public:

private:
    
};

// ---------------------------------------------------------------------------- VIEW
class View {
public:
    View() = default;

    void add(Element* element) {}
    void add(std::vector<Element*> elements) {}

    void update() {}
    void render(sf::RenderTarget& target) const {}

private:
    std::vector<Element*> m_elements;

    void handleEvents() {}
};

} // !namespace uilo

#endif // !UILO_HPP