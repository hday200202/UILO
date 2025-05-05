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

inline Align operator|(Align lhs, Align rhs) {
    return static_cast<Align>(
        static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs)
    );
}

inline Align operator&(Align lhs, Align rhs) {
    return static_cast<Align>(
        static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs)
    );
}

inline bool hasAlign(Align value, Align flag) {
    return static_cast<uint8_t>(value & flag) != 0;
}

// ---------------------------------------------------------------------------- BASE
class Element {
public:
    Element(){}
    virtual ~Element() = default;

    virtual void update() {}
    virtual void render(sf::RenderTarget& target) const {}
    
    Element* onClick(std::function<void()>& func)       { m_onClick = func; return this; }
    Element* onHover(std::function<void()>& func)       { m_onHover = func; return this; }
    Element* onRelease(std::function<void()>& func)     { m_onRelease = func; return this; }

    Element* setFixedWidth(float width)                 { m_size.x = width; m_fixedWidth = true; return this; }
    Element* setFixedHeight(float height)               { m_size.y = height; m_fixedHeight = true; return this; }
    Element* setFixedSize(float width, float height)    { m_size = {width, height}; m_fixedWidth = m_fixedHeight = true; return this; }
    Element* setAlignment(Align align)                  { m_align = align; return this; }
    Element* setPosition(float x, float y)              { m_position = {x, y}; m_fixedXPos = m_fixedYPos = true; return this; }
    Element* setXPosition(float x)                      { m_position.x = x; m_fixedXPos = true; return this; }
    Element* setYPosition(float y)                      { m_position.y = y; m_fixedYPos = true; return this; }

    static inline void setReferenceViewSize(const sf::Vector2f& size) { referenceViewSize = size; }

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
    bool m_fixedXPos = false;
    bool m_fixedYPos = false;

    static inline sf::Vector2f referenceViewSize = {0.f, 0.f};
    mutable sf::Vector2f m_currentViewSize = {0.f, 0.f};

    sf::Vector2f getScaledPosition() const {
        if (referenceViewSize.x == 0.f || referenceViewSize.y == 0.f)
            return m_position; // fallback — avoid divide-by-zero

        sf::Vector2f currentSize = m_currentViewSize;
        return {
            m_position.x * (currentSize.x / referenceViewSize.x),
            m_position.y * (currentSize.y / referenceViewSize.y)
        };
    }

    void setCurrentViewSize(const sf::Vector2f& viewSize) const {
        m_currentViewSize = viewSize;
    }

    void applyPixelPerfectView(sf::RenderTarget& target) const {
        sf::Vector2f viewSize = static_cast<sf::Vector2f>(target.getSize());
        sf::View newView;
        newView.setSize(viewSize);
        newView.setCenter({viewSize.x / 2.f, viewSize.y / 2.f});
        target.setView(newView);
        setCurrentViewSize(viewSize); // combine logic here
    }

    sf::Vector2f align(sf::FloatRect bounds) const {
        // Get scaled position based on reference resolution
        sf::Vector2f newPosition = getScaledPosition();
        const sf::Vector2f viewSize = m_currentViewSize;
    
        // Horizontal alignment
        if (hasAlign(m_align, Align::CENTER_H))
            newPosition.x = (viewSize.x - bounds.size.x) / 2.f - bounds.position.x;
        else if (hasAlign(m_align, Align::RIGHT))
            newPosition.x = viewSize.x - bounds.size.x - bounds.position.x;
        else if (hasAlign(m_align, Align::LEFT))
            newPosition.x = -bounds.position.x;
    
        // Vertical alignment
        if (hasAlign(m_align, Align::CENTER_V))
            newPosition.y = (viewSize.y - bounds.size.y) / 2.f - bounds.position.y;
        else if (hasAlign(m_align, Align::BOTTOM))
            newPosition.y = viewSize.y - bounds.size.y - bounds.position.y;
        else if (hasAlign(m_align, Align::TOP))
            newPosition.y = -bounds.position.y;

        return newPosition;
    }

private:

};

// ---------------------------------------------------------------------------- DESIGN / LAYOUT
class Text : public Element {
public:
    Text(const std::string& text = "", const std::string& fontPath = "", uint8_t size = 12, Align align = Align::NONE)
    : m_text(text), m_size(size) { if (!fontPath.empty()) m_font.openFromFile(fontPath); }
    ~Text() {}

    void update() {}
    void render(sf::RenderTarget& target) const override {
        sf::Text drawText(m_font, m_text, m_size);
        sf::FloatRect bounds = drawText.getLocalBounds();
        
        // Scale and align element
        applyPixelPerfectView(target);
        auto newPosition = align(bounds);
        
        drawText.setOrigin(m_position);
        drawText.setPosition(newPosition);
        target.draw(drawText);
    }

private:
    sf::Font m_font;
    sf::String m_text;
    uint8_t m_size;
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
    View(sf::RenderTarget* target)
        : m_target(target) {
            if (m_target)
                Element::setReferenceViewSize(m_target->getView().getSize());
        }

    void add(Element* element) {
        m_elements.push_back(element);
    }

    void add(const std::vector<Element*>& elements) {
        m_elements.insert(m_elements.end(), elements.begin(), elements.end());
    }

    void update() {
        if (!m_target) return;

        sf::Vector2f viewSize = static_cast<sf::Vector2f>(m_target->getSize());
        sf::View fixedView;
        fixedView.setSize(viewSize);
        fixedView.setCenter({viewSize.x / 2.f, viewSize.y / 2.f});
        m_target->setView(fixedView);

        for (auto* e : m_elements)
            e->update();
    }

    void render() const {
        if (!m_target) return;

        for (auto* e : m_elements)
            e->render(*m_target);
    }

private:
    sf::RenderTarget* m_target = nullptr;
    std::vector<Element*> m_elements;

    void handleEvents() {} // stub for later
};
    

} // !namespace uilo

#endif // !UILO_HPP