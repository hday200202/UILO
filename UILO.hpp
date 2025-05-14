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
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <future>

namespace uilo {

// ---------------------------------------------------------------------------- //
// Forward Declarations
// ---------------------------------------------------------------------------- //
class Element;
class Container;
class Row;
class Column;
class Page;
class UILO;



// ---------------------------------------------------------------------------- //
// Global Ownership
// ---------------------------------------------------------------------------- //
static std::unordered_set<Page*> uilo_owned_pages;
static std::vector<std::unique_ptr<Element>> uilo_owned_elements;
static bool time_to_delete = false;



// ---------------------------------------------------------------------------- //
// Alignment Enum
// ---------------------------------------------------------------------------- //
enum class Align : uint8_t {
    NONE      = 0,
    TOP       = 1 << 0,
    BOTTOM    = 1 << 1,
    LEFT      = 1 << 2,
    RIGHT     = 1 << 3,
    CENTER_X  = 1 << 4,
    CENTER_Y  = 1 << 5,
};

inline Align operator|(Align lhs, Align rhs);
inline Align operator&(Align lhs, Align rhs);
inline bool hasAlign(Align value, Align flag);



// ---------------------------------------------------------------------------- //
// Element Type Enum
// ---------------------------------------------------------------------------- //
enum class EType {
    Element,
    Row,
    Column,
    Text,
    Button,
};



// ---------------------------------------------------------------------------- //
// Button Styles
// ---------------------------------------------------------------------------- //
enum class ButtonStyle {
    Default,
    Pill,
    Rect,
};



// ---------------------------------------------------------------------------- //
// Modifier
// ---------------------------------------------------------------------------- //
using funcPtr = std::function<void()>;

class Modifier {
public:
    Modifier() = default;

    Modifier& setWidth(float pct);
    Modifier& setHeight(float pct);
    Modifier& setfixedWidth(float px);
    Modifier& setfixedHeight(float px);
    Modifier& align(Align alignment);
    Modifier& setColor(sf::Color color);
    Modifier& onClick(funcPtr cb);
    Modifier& setVisible(bool visible);

    float getWidth() const;
    float getHeight() const;
    float getFixedWidth() const;
    float getFixedHeight() const;
    Align getAlignment() const;
    sf::Color getColor() const;
    const funcPtr& getOnClick() const;
    bool isVisible() const;

private:
    float m_widthPct = 1.f;
    float m_heightPct = 1.f;
    float m_fixedWidth = 0.f;
    float m_fixedHeight = 0.f;
    bool m_isVisible = true;
    Align m_alignment = Align::NONE;
    sf::Color m_color = sf::Color::Transparent;
    std::function<void()> m_onClick = nullptr;
};

inline Modifier default_mod;



// ---------------------------------------------------------------------------- //
// Element Base Class
// ---------------------------------------------------------------------------- //
class Element {
public:
    sf::RectangleShape m_bounds;
    Modifier m_modifier;
    bool m_uiloOwned = false;

    Element();
    virtual ~Element();

    virtual void update(sf::RectangleShape& parentBounds);
    virtual void render(sf::RenderTarget& target);
    virtual void handleEvent(const sf::Event& event);
    virtual void checkClick(const sf::Vector2f& pos);
    void setModifier(const Modifier& modifier);
    virtual EType getType() const;

protected:
    void resize(sf::RectangleShape& parent);
    void applyModifiers();
};



// ---------------------------------------------------------------------------- //
// Container Base Class
// ---------------------------------------------------------------------------- //

class Container : public Element {
public:
    Container(std::initializer_list<Element*> elements);
    Container(Modifier modifier, std::initializer_list<Element*> elements);
    ~Container();

    void addElement(Element* element);
    void addElements(std::initializer_list<Element*> elements);
    virtual void handleEvent(const sf::Event& event) override;
    const std::vector<Element*>& getElements() const;

protected:
    std::vector<Element*> m_elements;
};



// ---------------------------------------------------------------------------- //
// Row Container
// ---------------------------------------------------------------------------- //
class Row : public Container {
public:
    using Container::Container;

    void update(sf::RectangleShape& parentBounds) override;
    void render(sf::RenderTarget& target) override;
    void checkClick(const sf::Vector2f& pos) override;
    virtual EType getType() const override;

private:
    inline void applyVerticalAlignment(Element* e, const sf::RectangleShape& parentBounds);
};



// ---------------------------------------------------------------------------- //
// Column Container
// ---------------------------------------------------------------------------- //
class Column : public Container {
public:
    using Container::Container;

    void update(sf::RectangleShape& parentBounds) override;
    void render(sf::RenderTarget& target) override;
    void checkClick(const sf::Vector2f& pos) override;
    virtual EType getType() const override;

private:
    inline void applyHorizontalAlignment(Element* e, const sf::RectangleShape& parentBounds);
};
    



// ---------------------------------------------------------------------------- //
// Text Element (WIP / Placeholder)
// ---------------------------------------------------------------------------- //
class Text : public Element {
public:
    Text(Modifier modifier = default_mod, const std::string& str = "", sf::Font font = sf::Font());
    Text(Modifier modifier = default_mod, const std::string& str = "", const std::string& fontPath = "");
    void update(sf::RectangleShape& parentBounds) override;
    void render(sf::RenderTarget& target) override;

    void setString(const std::string& newStr);

private:
    std::string m_string = "";
    sf::Font m_font;
    std::optional<sf::Text> m_text;
};



// ---------------------------------------------------------------------------- //
// Spacer Element
// ---------------------------------------------------------------------------- //
class Spacer : public Element {
public:
    Spacer(Modifier& modifier);
    void update(sf::RectangleShape& parentBounds) override;
};



// ---------------------------------------------------------------------------- //
// Button Element
// ---------------------------------------------------------------------------- //
class Button : public Element {
public:
    Button(
        Modifier modifier = default_mod,
        ButtonStyle buttonStyle = ButtonStyle::Default,
        const std::string& buttonText = "",
        const std::string& textFont = "",
        sf::Color textColor = sf::Color::White
    );

    void update(sf::RectangleShape& parentBounds) override;
    void render (sf::RenderTarget& target) override;
    void checkClick(const sf::Vector2f& pos) override;

    void setText(const std::string& newStr);

private:
    ButtonStyle m_buttonStyle = ButtonStyle::Default;

    sf::RectangleShape m_bodyRect;
    sf::CircleShape m_leftCircle;
    sf::CircleShape m_rightCircle;

    Text* m_text;
};



// ---------------------------------------------------------------------------- //
// Page View
// ---------------------------------------------------------------------------- //
class Page {
public:
    Page() = default;

    Page(std::initializer_list<Container*> containers = {});
    ~Page();

    void update(const sf::RectangleShape& parentBounds);
    void render(sf::RenderTarget& target);
    void handleEvent(const sf::Event& event);
    void dispatchClick(const sf::Vector2f& pos);

private:
    std::vector<Container*> m_containers;
    sf::RectangleShape m_bounds;
};



// ---------------------------------------------------------------------------- //
// UILO Application Core
// ---------------------------------------------------------------------------- //
class UILO {
public:
    UILO();
    UILO(const std::string& windowTitle = "", std::initializer_list<std::pair<Page*, std::string>> pages = {});
    UILO(sf::RenderWindow& userWindow, sf::View& windowView, std::initializer_list<std::pair<Page*, std::string>> pages = {});
    ~UILO();

    void update();
    void update(sf::View& windowView);
    void render();
    void setTitle(const std::string& newTitle);
    bool isRunning() const;
    void addPage(std::pair<Page*&, std::string> newPage);
    void addPages(std::initializer_list<std::pair<Page*&, std::string>> pages);
    void switchToPage(const std::string& pageName);

private:
    sf::RenderWindow m_window;
    sf::RenderWindow* m_userWindow = nullptr;
    bool m_windowOwned = true;
    int m_pollCount = 10;
    sf::VideoMode m_defScreenRes;
    sf::View m_defaultView;
    sf::RectangleShape m_bounds;
    std::string m_windowTitle = "";

    std::unordered_map<std::string, Page*> m_pages;
    Page* m_currentPage = nullptr;

    bool m_running       = false;
    bool m_shouldUpdate  = true;

    sf::Vector2u m_lastWindowSize;
    std::optional<sf::Vector2f> m_clickPosition;

    std::vector<std::unique_ptr<Page>> m_ownedPages;

    void pollEvents();
    void initDefaultView();
    void setView(const sf::View& view);
};



// ---------------------------------------------------------------------------- //
// Modifier Implementation
// ---------------------------------------------------------------------------- //
inline Modifier& Modifier::setWidth(float pct)          { m_widthPct = pct; m_fixedWidth = 0.f; return *this; }
inline Modifier& Modifier::setHeight(float pct)         { m_heightPct = pct; m_fixedHeight = 0.f; return *this; }
inline Modifier& Modifier::setfixedWidth(float px)      { m_fixedWidth = px; return *this; }
inline Modifier& Modifier::setfixedHeight(float px)     { m_fixedHeight = px; return *this; }
inline Modifier& Modifier::align(Align alignment)       { m_alignment = alignment; return *this; }
inline Modifier& Modifier::setColor(sf::Color color)    { m_color = color; return *this; }
inline Modifier& Modifier::onClick(funcPtr cb)          { m_onClick = std::move(cb); return *this; }
inline Modifier& Modifier::setVisible(bool visible)     { m_isVisible = visible; return *this; }

inline float Modifier::getWidth() const                 { return m_widthPct; }
inline float Modifier::getHeight() const                { return m_heightPct; }
inline float Modifier::getFixedWidth() const            { return m_fixedWidth; }
inline float Modifier::getFixedHeight() const           { return m_fixedHeight; }
inline Align Modifier::getAlignment() const             { return m_alignment; }
inline sf::Color Modifier::getColor() const             { return m_color; }
inline const funcPtr& Modifier::getOnClick() const      { return m_onClick; }
inline bool Modifier::isVisible() const                 { return m_isVisible; }



// ---------------------------------------------------------------------------- //
// Element Implementation
// ---------------------------------------------------------------------------- //
inline Element::Element() {}
inline Element::~Element() {}

inline void Element::update(sf::RectangleShape& parentBounds) {}
inline void Element::render(sf::RenderTarget& target) {}

inline void Element::handleEvent(const sf::Event& event) {
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePressed->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos(mousePressed->position);
            if (m_bounds.getGlobalBounds().contains(mousePos)) {
                if (m_modifier.getOnClick())
                    m_modifier.getOnClick()();
            }
        }
    }
}

inline void Element::checkClick(const sf::Vector2f& pos) {
    if (m_bounds.getGlobalBounds().contains(pos)) {
        if (m_modifier.getOnClick())
            m_modifier.getOnClick()();
    }
}

inline void Element::setModifier(const Modifier& modifier) {
    m_modifier = modifier;
}

inline EType Element::getType() const {
    return EType::Element;
}

inline void Element::resize(sf::RectangleShape& parent) {
    if (m_modifier.getFixedWidth() != 0) {
        m_bounds.setSize({
            m_modifier.getFixedWidth(),
            m_bounds.getSize().y
        });
    }
    else {
        m_bounds.setSize({
            m_modifier.getWidth() * parent.getSize().x,
            m_bounds.getSize().y
        });
    }

    if (m_modifier.getFixedHeight() != 0) {
        m_bounds.setSize({
            m_bounds.getSize().x,
            m_modifier.getFixedHeight()
        });
    }
    else {
        m_bounds.setSize({
            m_bounds.getSize().x,
            m_modifier.getHeight() * parent.getSize().y
        });
    }
}

inline void Element::applyModifiers() {
    m_bounds.setFillColor(m_modifier.getColor());
}



// ---------------------------------------------------------------------------- //
// Container Implementation
// ---------------------------------------------------------------------------- //
inline Container::Container(std::initializer_list<Element*> elements) {
    for (auto& e : elements)
        m_elements.push_back(e);
}

inline Container::Container(Modifier modifier, std::initializer_list<Element*> elements) {
    m_modifier = modifier;

    for (auto& e : elements)
        m_elements.push_back(e);   
}

inline Container::~Container() {}

inline void Container::addElement(Element* element) {
    m_elements.push_back(element);
}

inline void Container::addElements(std::initializer_list<Element*> elements) {
    for (auto& e : elements)
        m_elements.push_back(e);
}

inline void Container::handleEvent(const sf::Event& event) {
    for (auto& e : m_elements)
        e->handleEvent(event);
    Element::handleEvent(event);
}

inline const std::vector<Element*>& Container::getElements() const {
    return m_elements;
}



// ---------------------------------------------------------------------------- //
// Row Implementation
// ---------------------------------------------------------------------------- //
inline void Row::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    applyModifiers();

    std::vector<Element*> left, center, right;
    for (auto& e : m_elements) {
        Align a = e->m_modifier.getAlignment();
        if (hasAlign(a, Align::RIGHT)) right.push_back(e);
        else if (hasAlign(a, Align::CENTER_X)) center.push_back(e);
        else left.push_back(e); // default is LEFT
    }

    float totalFixed = 0.f;
    float totalPercent = 0.f;

    auto measureWidth = [&](Element* e) {
        if (e->m_modifier.getFixedWidth() > 0.f)
            return e->m_modifier.getFixedWidth();
        totalPercent += e->m_modifier.getWidth();
        return 0.f;
    };

    for (auto& e : m_elements)
        totalFixed += measureWidth(e);

    float remaining = m_bounds.getSize().x - totalFixed;
    float centerWeight = 0.f;
    for (auto& e : center) centerWeight += e->m_modifier.getWidth();

    // LEFT-aligned
    float xLeft = m_bounds.getPosition().x;
    for (auto& e : left) {
        float width = (e->m_modifier.getFixedWidth() > 0.f)
            ? e->m_modifier.getFixedWidth()
            : (e->m_modifier.getWidth() / totalPercent) * remaining;

        sf::RectangleShape slot({ width, m_bounds.getSize().y });
        slot.setPosition({ xLeft, m_bounds.getPosition().y });

        e->update(slot);
        e->m_bounds.setPosition({ xLeft, m_bounds.getPosition().y }); // Y alignment later
        xLeft += e->m_bounds.getSize().x;
    }

    // RIGHT-aligned
    float xRight = m_bounds.getPosition().x + m_bounds.getSize().x;
    for (auto it = right.rbegin(); it != right.rend(); ++it) {
        Element* e = *it;
        float width = (e->m_modifier.getFixedWidth() > 0.f)
            ? e->m_modifier.getFixedWidth()
            : (e->m_modifier.getWidth() / totalPercent) * remaining;

        xRight -= width;
        sf::RectangleShape slot({ width, m_bounds.getSize().y });
        slot.setPosition({ xRight, m_bounds.getPosition().y });

        e->update(slot);
        e->m_bounds.setPosition({ xRight, m_bounds.getPosition().y }); // Y alignment later
    }

    // CENTER-aligned
    float centerTotalWidth = 0.f;
    for (auto& e : center) {
        float width = (e->m_modifier.getFixedWidth() > 0.f)
            ? e->m_modifier.getFixedWidth()
            : (e->m_modifier.getWidth() / totalPercent) * remaining;
        centerTotalWidth += width;
    }

    float xCenter = m_bounds.getPosition().x + (m_bounds.getSize().x - centerTotalWidth) / 2.f;
    for (auto& e : center) {
        float width = (e->m_modifier.getFixedWidth() > 0.f)
            ? e->m_modifier.getFixedWidth()
            : (e->m_modifier.getWidth() / totalPercent) * remaining;

        sf::RectangleShape slot({ width, m_bounds.getSize().y });
        slot.setPosition({ xCenter, m_bounds.getPosition().y });

        e->update(slot);
        e->m_bounds.setPosition({ xCenter, m_bounds.getPosition().y });
        xCenter += e->m_bounds.getSize().x;
    }

    // Then apply vertical (Y) alignment if needed
    for (auto& e : m_elements) {
        auto a = e->m_modifier.getAlignment();
        auto pos = e->m_bounds.getPosition();

        if (hasAlign(a, Align::CENTER_Y))
            pos.y = m_bounds.getPosition().y + (m_bounds.getSize().y - e->m_bounds.getSize().y) / 2.f;
        else if (hasAlign(a, Align::BOTTOM))
            pos.y = m_bounds.getPosition().y + m_bounds.getSize().y - e->m_bounds.getSize().y;

        e->m_bounds.setPosition(pos);
    }
}    

inline void Row::render(sf::RenderTarget& target) {
    target.draw(m_bounds);
    for (auto& e : m_elements) {
        if (e->m_modifier.isVisible())
            e->render(target);
    }
}

inline void Row::checkClick(const sf::Vector2f& pos) {
    for (auto& e : m_elements)
        e->checkClick(pos);
    Element::checkClick(pos);
}

inline EType Row::getType() const {
    return EType::Row;
}

inline void Row::applyVerticalAlignment(Element* e, const sf::RectangleShape& parentBounds) {
    auto align = e->m_modifier.getAlignment();
    auto pos = e->m_bounds.getPosition();

    if (hasAlign(align, Align::CENTER_Y))
        pos.y = parentBounds.getPosition().y + (parentBounds.getSize().y - e->m_bounds.getSize().y) / 2.f;
    else if (hasAlign(align, Align::BOTTOM))
        pos.y = parentBounds.getPosition().y + parentBounds.getSize().y - e->m_bounds.getSize().y;

    e->m_bounds.setPosition(pos);
}



// ---------------------------------------------------------------------------- //
// Column Implementation
// ---------------------------------------------------------------------------- //
inline void Column::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    applyModifiers();

    std::vector<Element*> top, center, bottom;
    for (auto& e : m_elements) {
        Align a = e->m_modifier.getAlignment();
        if (hasAlign(a, Align::BOTTOM)) bottom.push_back(e);
        else if (hasAlign(a, Align::CENTER_Y)) center.push_back(e);
        else top.push_back(e); // default is TOP
    }

    float totalFixed = 0.f;
    float totalPercent = 0.f;

    auto measureHeight = [&](Element* e) {
        if (e->m_modifier.getFixedHeight() > 0.f)
            return e->m_modifier.getFixedHeight();
        totalPercent += e->m_modifier.getHeight();
        return 0.f;
    };

    for (auto& e : m_elements)
        totalFixed += measureHeight(e);

    float remaining = m_bounds.getSize().y - totalFixed;
    float centerWeight = 0.f;
    for (auto& e : center) centerWeight += e->m_modifier.getHeight();

    // TOP-aligned
    float yTop = m_bounds.getPosition().y;
    for (auto& e : top) {
        float height = (e->m_modifier.getFixedHeight() > 0.f)
            ? e->m_modifier.getFixedHeight()
            : (e->m_modifier.getHeight() / totalPercent) * remaining;

        sf::RectangleShape slot({ m_bounds.getSize().x, height });
        slot.setPosition({ m_bounds.getPosition().x, yTop });

        e->update(slot);
        e->m_bounds.setPosition({ m_bounds.getPosition().x, yTop }); // X alignment later
        yTop += e->m_bounds.getSize().y;
    }

    // BOTTOM-aligned
    float yBottom = m_bounds.getPosition().y + m_bounds.getSize().y;
    for (auto it = bottom.rbegin(); it != bottom.rend(); ++it) {
        Element* e = *it;
        float height = (e->m_modifier.getFixedHeight() > 0.f)
            ? e->m_modifier.getFixedHeight()
            : (e->m_modifier.getHeight() / totalPercent) * remaining;

        yBottom -= height;

        sf::RectangleShape slot({ m_bounds.getSize().x, height });
        slot.setPosition({ m_bounds.getPosition().x, yBottom });

        e->update(slot);
        e->m_bounds.setPosition({ m_bounds.getPosition().x, yBottom });
    }

    // CENTER-aligned
    float centerTotalHeight = 0.f;
    for (auto& e : center) {
        float height = (e->m_modifier.getFixedHeight() > 0.f)
            ? e->m_modifier.getFixedHeight()
            : (e->m_modifier.getHeight() / totalPercent) * remaining;
        centerTotalHeight += height;
    }

    float yCenter = m_bounds.getPosition().y + (m_bounds.getSize().y - centerTotalHeight) / 2.f;
    for (auto& e : center) {
        float height = (e->m_modifier.getFixedHeight() > 0.f)
            ? e->m_modifier.getFixedHeight()
            : (e->m_modifier.getHeight() / totalPercent) * remaining;

        sf::RectangleShape slot({ m_bounds.getSize().x, height });
        slot.setPosition({ m_bounds.getPosition().x, yCenter });

        e->update(slot);
        e->m_bounds.setPosition({ m_bounds.getPosition().x, yCenter });
        yCenter += e->m_bounds.getSize().y;
    }

    // Final pass for X alignment
    for (auto& e : m_elements) {
        Align a = e->m_modifier.getAlignment();
        sf::Vector2f pos = e->m_bounds.getPosition();

        if (hasAlign(a, Align::CENTER_X))
            pos.x = m_bounds.getPosition().x + (m_bounds.getSize().x - e->m_bounds.getSize().x) / 2.f;
        else if (hasAlign(a, Align::RIGHT))
            pos.x = m_bounds.getPosition().x + m_bounds.getSize().x - e->m_bounds.getSize().x;

        e->m_bounds.setPosition(pos);
    }
}    

inline void Column::render(sf::RenderTarget& target) {
    target.draw(m_bounds);
    for (auto& e : m_elements) {
        if (e->m_modifier.isVisible())
            e->render(target);
    }
}

inline void Column::checkClick(const sf::Vector2f& pos) {
    for (auto& e : m_elements)
        e->checkClick(pos);
    Element::checkClick(pos);
}

inline EType Column::getType() const {
    return EType::Column;
}

inline void Column::applyHorizontalAlignment(Element* e, const sf::RectangleShape& parentBounds) {
    auto align = e->m_modifier.getAlignment();
    auto pos = e->m_bounds.getPosition();

    if (hasAlign(align, Align::CENTER_X))
        pos.x = parentBounds.getPosition().x + (parentBounds.getSize().x - e->m_bounds.getSize().x) / 2.f;
    else if (hasAlign(align, Align::RIGHT))
        pos.x = parentBounds.getPosition().x + parentBounds.getSize().x - e->m_bounds.getSize().x;

    e->m_bounds.setPosition(pos);
}



// ---------------------------------------------------------------------------- //
// Text Implementation
// ---------------------------------------------------------------------------- //
inline Text::Text(Modifier modifier, const std::string& str, sf::Font font) 
: m_string(str), m_font(font) {
    m_modifier = modifier;
}

inline Text::Text(Modifier modifier, const std::string& str, const std::string& fontPath)
: m_string(str) {
    m_modifier = modifier;

    if (!fontPath.empty())
        if (!m_font.openFromFile(fontPath))
            std::cerr << "Text Error: couldn't load font \"" << fontPath << "\".\n";
}

inline void Text::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    m_bounds.setPosition(parentBounds.getPosition());  // ← 🔧 This is the key fix

    float fontSize = m_modifier.getFixedHeight() > 0
        ? m_modifier.getFixedHeight()
        : m_bounds.getSize().y;

    m_text.emplace(m_font, m_string);
    m_text->setCharacterSize(static_cast<unsigned>(fontSize));
    m_text->setFillColor(m_modifier.getColor());

    sf::FloatRect textBounds = m_text->getLocalBounds();

    // Resize m_bounds width based on text
    m_bounds.setSize({ textBounds.size.x + textBounds.position.x, m_bounds.getSize().y });
}

inline void Text::render(sf::RenderTarget& target) {
    if (!m_text) return;

    sf::FloatRect bounds = m_text->getLocalBounds();
    m_text->setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});

    m_text->setPosition(
        {m_bounds.getPosition().x + m_bounds.getSize().x / 2.f,
        m_bounds.getPosition().y + m_bounds.getSize().y / 2.f}
    );

    target.draw(*m_text);
}

inline void Text::setString(const std::string& newStr) {
    m_text->setString(newStr);
}



// ---------------------------------------------------------------------------- //
// Spacer Implementation
// ---------------------------------------------------------------------------- //
inline Spacer::Spacer(Modifier& modifier) { 
    m_modifier = modifier;
}

inline void Spacer::update(sf::RectangleShape& parentBounds) {
    m_bounds.setFillColor(sf::Color::Transparent);

    resize(parentBounds);
    applyModifiers();
}



// ---------------------------------------------------------------------------- //
// Button Implementation
// ---------------------------------------------------------------------------- //
inline Button::Button(
    Modifier modifier, 
    ButtonStyle buttonStyle, 
    const std::string& buttonText,
    const std::string& textFont,
    sf::Color textColor
) {
    m_modifier = modifier;
    m_buttonStyle = buttonStyle;

    m_bodyRect.setFillColor(m_modifier.getColor());
    m_leftCircle.setFillColor(m_modifier.getColor());
    m_rightCircle.setFillColor(m_modifier.getColor());

    if (!textFont.empty()) {
        m_text = new Text(
            Modifier()
            .setHeight(0.4f)
            .setColor(textColor)
            .align(Align::CENTER_X | Align::CENTER_Y), 
            buttonText, 
            textFont
        );
    }
}

inline void Button::update(sf::RectangleShape& parentBounds) {
    applyModifiers();
    m_bounds.setSize({
        m_modifier.getFixedWidth() ? m_modifier.getFixedWidth() : parentBounds.getSize().x * m_modifier.getWidth(),
        m_modifier.getFixedHeight() ? m_modifier.getFixedHeight() : parentBounds.getSize().y * m_modifier.getHeight()
    });

    // m_bounds.setPosition(parentBounds.getPosition());

    if (m_text) {
        m_text->update(m_bounds);
        m_text->m_bounds.setPosition({
            m_bounds.getPosition().x + (m_bounds.getSize().x / 2.f) - (m_text->m_bounds.getSize().x / 2.f),
            m_bounds.getPosition().y + (m_bounds.getSize().y / 2.f) - (m_text->m_bounds.getSize().y / 2.f)
        });
    }
}

inline void Button::render (sf::RenderTarget& target) {
    if (m_buttonStyle == ButtonStyle::Default || m_buttonStyle == ButtonStyle::Rect)
        target.draw(m_bounds);

    else {
        m_leftCircle.setPointCount(static_cast<unsigned int>(m_bounds.getSize().y * 2));
        m_rightCircle.setPointCount(static_cast<unsigned int>(m_bounds.getSize().y * 2));

        m_leftCircle.setRadius(m_bounds.getSize().y / 2);
        m_rightCircle.setRadius(m_bounds.getSize().y / 2);
        m_bodyRect.setSize
        ({
            m_bounds.getSize().x - m_bounds.getSize().y, 
            m_bounds.getSize().y
        });
        m_leftCircle.setPosition(m_bounds.getPosition());
        m_rightCircle.setPosition
        ({
            m_bounds.getPosition().x + m_bounds.getSize().x - m_bounds.getSize().y, 
            m_bounds.getPosition().y
        });
        m_bodyRect.setPosition
        ({
            m_bounds.getPosition().x + m_leftCircle.getRadius(),
            m_bounds.getPosition().y
        });

        target.draw(m_leftCircle);

        if (!(m_leftCircle.getPosition() == m_rightCircle.getPosition())) {
            target.draw(m_rightCircle);
            target.draw(m_bodyRect);
        }
    }
    
    if (m_text) {
        m_text->render(target);
    }
}

inline void Button::checkClick(const sf::Vector2f& pos) {
    if (m_bounds.getGlobalBounds().contains(pos)) {
        if (m_modifier.getOnClick()) m_modifier.getOnClick()();
    }
}


inline void Button::setText(const std::string& newStr) {
    if (m_text)
        m_text->setString(newStr);
}



// ---------------------------------------------------------------------------- //
// Element Factory
// ---------------------------------------------------------------------------- //
template <typename T, typename... Args>
T* obj(Args&&... args) {
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    T* raw = ptr.get();
    uilo_owned_elements.emplace_back(std::move(ptr));
    return raw;
}

inline Row* row(auto&&... args) {
    return obj<Row>(std::forward<decltype(args)>(args)...);
}

inline Column* column(auto&&... args) {
    return obj<Column>(std::forward<decltype(args)>(args)...);
}

inline Spacer* spacer(auto&&... args) {
    return obj<Spacer>(std::forward<decltype(args)>(args)...);
}

inline Button* button(auto&&... args) {
    return obj<Button>(std::forward<decltype(args)>(args)...);
}

inline Text* text(auto&&... args) {
    return obj<Text>(std::forward<decltype(args)>(args)...);
}

// ---------------------------------------------------------------------------- //
// Page Implementation
// ---------------------------------------------------------------------------- //
inline Page::Page(std::initializer_list<Container*> containers) {
    m_bounds.setFillColor(sf::Color::Transparent);

    for (const auto& c : containers) {
        m_containers.push_back(c);
    }

    uilo_owned_pages.insert(this);
}
 
inline Page::~Page() {
    if (uilo_owned_pages.find(this) != uilo_owned_pages.end() && !time_to_delete) {
        std::cerr << "[UILO] Error: Attempted to delete a UILO-owned page directly.\n";
        std::abort();
    }
}
 
inline void Page::update(const sf::RectangleShape& parentBounds) {
    m_bounds = parentBounds;

    std::vector<std::future<void>> futures;

    for (auto& c : m_containers) {
        c->m_bounds.setPosition(m_bounds.getPosition());
        c->update(m_bounds);
    }
}
 
inline void Page::render(sf::RenderTarget& target) {
    target.draw(m_bounds);

    for (auto& c : m_containers) {
        if (c->m_modifier.isVisible())
            c->render(target);
    }
}
 
inline void Page::handleEvent(const sf::Event& event) {
    for (auto& c : m_containers)
        c->handleEvent(event);
}
 
inline void Page::dispatchClick(const sf::Vector2f& pos) {
    for (auto& c : m_containers)
        c->checkClick(pos);
}

inline Page* page(std::initializer_list<Container*> containers = {}) {
    return new Page(containers); // Let Page's constructor handle insertion into uilo_owned_pages
}



// ---------------------------------------------------------------------------- //
// UILO Implementation
// ---------------------------------------------------------------------------- //
inline UILO::UILO() {
    initDefaultView();
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 4;

    m_window.create(
        m_defScreenRes, m_windowTitle,
        sf::Style::Resize | sf::Style::Close,
        sf::State::Windowed,
        settings
    );

    m_window.setVerticalSyncEnabled(true);
    m_window.setView(m_defaultView);

    if (m_window.isOpen()) m_running = true;

    m_bounds.setFillColor(sf::Color::Transparent);
}

inline UILO::UILO(const std::string& windowTitle, std::initializer_list<std::pair<Page*, std::string>> pages)
: m_windowTitle(windowTitle) {
    initDefaultView();
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 4;

    m_window.create(
        m_defScreenRes, m_windowTitle,
        sf::Style::Resize | sf::Style::Close,
        sf::State::Windowed,
        settings
    );

    m_window.setVerticalSyncEnabled(true);
    m_window.setView(m_defaultView);

    if (m_window.isOpen()) m_running = true;

    for (auto& [page, name] : pages) {
        if (uilo_owned_pages.find(page) != uilo_owned_pages.end()) {
            uilo_owned_pages.insert(page);
        }
        m_ownedPages.push_back(std::unique_ptr<Page>(page));
        m_pages[name] = page;
        if (!m_currentPage) m_currentPage = page;
    }

    m_bounds.setFillColor(sf::Color::Transparent);
}

inline UILO::UILO(sf::RenderWindow& userWindow, sf::View& windowView, std::initializer_list<std::pair<Page*, std::string>> pages) {
    m_defaultView = windowView;
    m_userWindow = &userWindow;
    m_userWindow->setView(m_defaultView);

    if (m_userWindow->isOpen()) {
        m_running = true;
        m_windowOwned = false;
    }

    for (auto& [page, name] : pages) {
        if (uilo_owned_pages.find(page) != uilo_owned_pages.end()) {
            uilo_owned_pages.insert(page);
        }
        m_ownedPages.push_back(std::unique_ptr<Page>(page));
        m_pages[name] = page;
        if (!m_currentPage) m_currentPage = page;

    }

    m_bounds.setFillColor(sf::Color::Transparent);
}

inline UILO::~UILO() {
    time_to_delete = true;
    uilo_owned_elements.clear();
    uilo_owned_pages.clear();
    m_pages.clear();
}

inline void UILO::update() {
    pollEvents();

    if (!m_windowOwned)
        return;

    sf::Vector2u currentSize = m_window.getSize();
    if (currentSize != m_lastWindowSize) {
        m_shouldUpdate = true;
        m_pollCount = 5;
    }

    if (m_shouldUpdate) {
        m_defaultView.setSize({ (float)currentSize.x, (float)currentSize.y });

        m_bounds.setSize(m_defaultView.getSize());
        m_bounds.setPosition({
            m_defaultView.getCenter().x - (m_defaultView.getSize().x / 2),
            m_defaultView.getCenter().y - (m_defaultView.getSize().y / 2)
        });

        m_window.setView(m_defaultView);
        m_currentPage->update(m_bounds);
        m_lastWindowSize = currentSize;
        render();
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (m_clickPosition) {
        m_currentPage->dispatchClick(*m_clickPosition);
        m_clickPosition.reset();
    }
}    

inline void UILO::update(sf::View& windowView) {
    pollEvents();

    if (m_windowOwned)
        return;

    sf::Vector2u currentSize = m_userWindow->getSize();
    if (m_shouldUpdate) {
        windowView.setSize({ (float)currentSize.x * 2, (float)currentSize.y * 2 });

        m_bounds.setSize(windowView.getSize());
        m_bounds.setPosition({
            windowView.getCenter().x - (windowView.getSize().x / 2),
            windowView.getCenter().y - (windowView.getSize().y / 2)
        });

        m_userWindow->setView(windowView);
        m_currentPage->update(m_bounds);
        m_lastWindowSize = currentSize;
    }
    else
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    if (m_clickPosition) {
        m_currentPage->dispatchClick(*m_clickPosition);
        m_clickPosition.reset();
    }
}

inline void UILO::render() {
    if (m_windowOwned) {
        if (m_pollCount == 1) {
            m_window.clear(sf::Color::Black);
            m_currentPage->render(m_window);
            m_window.display();
        }
    }
    else
        m_currentPage->render(*m_userWindow);

    m_shouldUpdate = false;
}

inline void UILO::setTitle(const std::string& newTitle) {
    m_window.setTitle(newTitle);
}

inline bool UILO::isRunning() const {
    return m_running;
}

inline void UILO::addPage(std::pair<Page*&, std::string> newPage) {
    Page*& page = newPage.first;
    const std::string& name = newPage.second;

    if (uilo_owned_pages.find(page) == uilo_owned_pages.end()) {
        uilo_owned_pages.insert(page);
    }
    m_ownedPages.push_back(std::unique_ptr<Page>(page));
    m_pages[name] = page;
    if (!m_currentPage) m_currentPage = page;
}

inline void UILO::addPages(std::initializer_list<std::pair<Page*&, std::string>> pages) {
    for (const auto& [page, name] : pages) {
        if (uilo_owned_pages.find(page) == uilo_owned_pages.end()) {
            uilo_owned_pages.insert(page);
        }
        m_ownedPages.push_back(std::unique_ptr<Page>(page));
        m_pages[name] = page;
        if (!m_currentPage) m_currentPage = page;
    }
}

inline void UILO::switchToPage(const std::string& pageName) {
    auto it = m_pages.find(pageName);
    if (it != m_pages.end()) {
        m_currentPage = it->second;
        m_currentPage->update(m_bounds);
    } else {
        std::cerr << "[UILO] Page \"" << pageName << "\" not found.\n";
    }
}

inline void UILO::pollEvents() {
    while (const auto event = (m_windowOwned) ?  m_window.pollEvent() : m_userWindow->pollEvent()) {
        if (!m_windowOwned && !m_userWindow)
            return;

        if (event->is<sf::Event::Closed>()) {
            if (m_windowOwned) m_window.close();
            else m_userWindow->close();
            m_running = false;
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mousePressed->button == sf::Mouse::Button::Left) {
                if (m_windowOwned) m_clickPosition = m_window.mapPixelToCoords(mousePressed->position);
                else m_clickPosition = m_userWindow->mapPixelToCoords(mousePressed->position);
            }

            m_shouldUpdate = true;
        }
    }

    // Let's the ui update 10 times at start
    if (m_pollCount != 0) {
        m_shouldUpdate = true;
        m_pollCount--;
    }
}

inline void UILO::initDefaultView() {
    m_defScreenRes = sf::VideoMode::getDesktopMode();
    m_defScreenRes.size.x /= 2;
    m_defScreenRes.size.y /= 2;

    m_defaultView.setSize({
        (float)m_defScreenRes.size.x,
        (float)m_defScreenRes.size.y
    });
}

inline void UILO::setView(const sf::View& view) {
    m_defaultView = view;
    if (!m_windowOwned)
        m_userWindow->setView(m_defaultView);
}



// ---------------------------------------------------------------------------- //
// Alignment Helpers
// ---------------------------------------------------------------------------- //
inline Align operator|(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline Align operator&(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool hasAlign(Align value, Align flag) {
    return static_cast<uint8_t>(value & flag) != 0;
}



using contains = std::initializer_list<uilo::Element*>;

} // !namespace uilo

#endif // !UILO_HPP
