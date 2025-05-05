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

    virtual void update(float dt) {}
    virtual void render(sf::RenderTarget& target) const {}

    std::function<void()> onClick = nullptr;
    std::function<void()> onHover = nullptr;
    std::function<void()> onRelease = nullptr;

    bool fixedWidth = false;
    bool fixedHeigt = false;
    bool fixedSize = false;

protected:
    sf::Vector2f m_position;
    sf::Vector2f m_size;

    float aspectRatio = 1.f;

private:

};

// ---------------------------------------------------------------------------- DESIGN / LAYOUT
class Text : public Element {

};

class Spacer : public Element {

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
    void update(float dt) {}
    void draw(sf::RenderTarget& target) const {}
    void handleEvent(const sf::Event& event) {}

private:
    std::vector<Element*> m_elements;
};

} // !namespace uilo

#endif // !UILO_HPP