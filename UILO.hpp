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
#include <memory>

namespace uilo {

// ---------------------------------------------------------------------------- Alignment
enum class Align : uint8_t 
{
    NONE = 0,
    TOP = 1 << 0,
    BOTTOM = 1 << 1,
    LEFT = 1 << 2,
    RIGHT = 1 << 3,
    CENTER_X = 1 << 4,
    CENTER_Y = 1 << 5,
};

inline Align operator|(Align lhs, Align rhs) 
{
    return static_cast<Align>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline Align operator&(Align lhs, Align rhs) 
{
    return static_cast<Align>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool hasAlign(Align value, Align flag) 
{
    return static_cast<uint8_t>(value & flag) != 0;
}
// ---------------------------------------------------------------------------- //



// ---------------------------------------------------------------------------- Modifier
class Modifier 
{
public:
    Modifier() {}

    Modifier& setWidth(float widthPct) { m_widthPct = widthPct; m_fixedWidth = 0.f; return *this; }
    Modifier& setHeight(float heightPct) { m_heightPct = heightPct; m_fixedHeight = 0.f; return *this; }
    Modifier& setfixedWidth(float width) { m_fixedWidth = width; return *this; }
    Modifier& setfixedHeight(float height) { m_fixedHeight = height; return *this; }
    Modifier& align(Align alignment) { m_alignment = alignment; return *this; }
    Modifier& setColor(sf::Color color) { m_color = color; return *this; }

    float getWidth() const { return m_widthPct; }
    float getHeight() const { return m_heightPct; }
    float getFixedWidth() const { return m_fixedWidth; }
    float getFixedHeight() const { return m_fixedHeight; }
    Align getAlignment() const { return m_alignment; }
    sf::Color getColor() const { return m_color; }

private:
    float m_widthPct = 1.f;
    float m_heightPct = 1.f;
    Align m_alignment = Align::NONE;
    sf::Color m_color = sf::Color::Transparent;
    float m_fixedWidth = 0.f;
    float m_fixedHeight = 0.f;
};

const Modifier default_mod;
// ----------------------------------------------------------------------------



// ---------------------------------------------------------------------------- Base
class Element 
{
public:

protected:
    
};
// ---------------------------------------------------------------------------- //



// ---------------------------------------------------------------------------- Containers
class Container : public Element 
{
public:

protected:
    
};



class Row : public Container 
{
public:
    
};



class Column : public Container 
{
public:
    
};
// ---------------------------------------------------------------------------- //



// ---------------------------------------------------------------------------- Text / Spacers
class Text : public Element {
public:

private:
    
};
// ---------------------------------------------------------------------------- //



// ---------------------------------------------------------------------------- View
class Page {
public:

private:
    
};
// ---------------------------------------------------------------------------- //



// ---------------------------------------------------------------------------- UILO
class UILO {
public:
    UILO() = default;
    UILO(std::initializer_list<std::pair<std::string, Page*>> pages) {}
    ~UILO() {};

    void update();
    void render();
    bool isRunning();

    void addPage(std::pair<std::string, Page*>& newPage) {}

private:
    sf::RenderWindow m_window;
    std::unordered_map<std::string, Page*> m_pages;
    Page* m_currentPage = nullptr;
};
// ---------------------------------------------------------------------------- //

} // !namespace uilo

#endif // !UILO_HPP