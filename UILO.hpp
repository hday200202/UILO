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
#include <algorithm>

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
class Slider;
class Text;
class Spacer;
class Button;



// ---------------------------------------------------------------------------- //
// Global Ownership
// ---------------------------------------------------------------------------- //
static std::unordered_set<Page*> uilo_owned_pages;
static std::vector<std::unique_ptr<Element>> uilo_owned_elements;
static bool time_to_delete = false;

static std::unordered_map<std::string, Slider*> sliders;
static std::unordered_map<std::string, Container*> containers;
static std::unordered_map<std::string, Text*> texts;
static std::unordered_map<std::string, Spacer*> spacers;
static std::unordered_map<std::string, Button*> buttons;

void cleanupMarkedElements();



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
    sf::RectangleShape m_pastBounds;
    Modifier m_modifier;
    bool m_uiloOwned = false;
    bool m_isDirty = false;
    bool m_markedForDeletion = false;

    std::string m_name = "";

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
    Container(std::initializer_list<Element*> elements = {}, const std::string& name = "");
    Container(Modifier modifier, std::initializer_list<Element*> elements = {}, const std::string& name = "");
    ~Container();

    void addElement(Element* element);
    void addElements(std::initializer_list<Element*> elements);
    virtual void handleEvent(const sf::Event& event) override;
    const std::vector<Element*>& getElements() const;
    void clear();

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
    Text(Modifier modifier = default_mod, const std::string& str = "", sf::Font font = sf::Font(), const std::string& name = "");
    Text(Modifier modifier = default_mod, const std::string& str = "", const std::string& fontPath = "", const std::string& name = "");
    void update(sf::RectangleShape& parentBounds) override;
    void render(sf::RenderTarget& target) override;

    void setString(const std::string& newStr);
    std::string getString() const { return m_text->getString(); }

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
    Spacer(Modifier& modifier, const std::string& name = "");
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
        sf::Color textColor = sf::Color::White,
        const std::string& name = ""
    );

    void update(sf::RectangleShape& parentBounds) override;
    void render (sf::RenderTarget& target) override;
    void checkClick(const sf::Vector2f& pos) override;

    void setText(const std::string& newStr);
    std::string getText() const;

    bool isClicked() const { return m_isClicked; }
    bool isHovered() const { return m_isHovered; }

    void setClicked(bool clicked) { m_isClicked = clicked; }

private:
    ButtonStyle m_buttonStyle = ButtonStyle::Default;

    sf::RectangleShape m_bodyRect;
    sf::CircleShape m_leftCircle;
    sf::CircleShape m_rightCircle;

    Text* m_text;

    bool m_isClicked = false;
    bool m_isHovered = false;
};



// ---------------------------------------------------------------------------- //
// Slider Element
// ---------------------------------------------------------------------------- //
class Slider : public Element {
public:
    Slider(
        Modifier modifier = default_mod,
        sf::Color knobColor = sf::Color::White,
        sf::Color barColor = sf::Color::Black,
        const std::string& name = ""
    );

    void update(sf::RectangleShape& parentBounds) override;
    void render(sf::RenderTarget& target) override;
    void checkClick(const sf::Vector2f& pos) override;

    float getValue() const;
    void setValue(float newVal);

private:
    float m_minVal = 0.f;
    float m_maxVal = 1.f;
    float m_curVal = 0.75f;

    sf::Color m_knobColor = sf::Color::White;
    sf::Color m_barColor = sf::Color::Black;

    sf::RectangleShape m_knobRect;
    sf::RectangleShape m_barRect;
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
    void clear();

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
    void addPage(std::pair<Page*, std::string> newPage);
    void addPages(std::initializer_list<std::pair<Page*, std::string>> pages);
    void switchToPage(const std::string& pageName);
    void forceUpdate();
    void setScale(float scale = 1.5f);

private:
    sf::RenderWindow m_window;
    sf::RenderWindow* m_userWindow = nullptr;
    bool m_windowOwned = true;
    int m_pollCount = 10;
    float m_renderScale = 1.5f;
    sf::VideoMode m_defScreenRes;
    sf::View m_defaultView;
    sf::RectangleShape m_bounds;
    std::string m_windowTitle = "";

    std::unordered_map<std::string, Page*> m_pages;
    Page* m_currentPage = nullptr;

    bool m_running       = false;
    bool m_shouldUpdate  = true;
    bool m_mouseDragging = false;

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

inline void Element::update(sf::RectangleShape& parentBounds) {
    // Base: just check dirty
    m_isDirty = (m_bounds.getPosition() != m_pastBounds.getPosition() || m_bounds.getSize() != m_pastBounds.getSize());
    m_pastBounds.setPosition(m_bounds.getPosition());
    m_pastBounds.setSize(m_bounds.getSize());
}
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
inline Container::Container(std::initializer_list<Element*> elements, const std::string& name) {
    for (auto& e : elements)
        m_elements.push_back(e);

    m_name = name;
    if (!m_name.empty()) {
        containers[m_name] = this;
    }
}

inline Container::Container(Modifier modifier, std::initializer_list<Element*> elements, const std::string& name) {
    m_modifier = modifier;

    for (auto& e : elements)
        m_elements.push_back(e);

    m_name = name;
    if (!m_name.empty()) {
        containers[m_name] = this;
    }
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

inline void Container::clear() {
    for (auto& e : m_elements) {
        if (Container* childContainer = dynamic_cast<Container*>(e)) {
            std::cout << "Clearing child container: " << childContainer->m_name << std::endl;
            childContainer->clear();
        }

        e->m_markedForDeletion = true;
    }
    m_elements.clear();

    cleanupMarkedElements();
}

inline void cleanupMarkedElements() {
    std::cout << "Cleaning up marked elements, uilo_owned_elements size: " << uilo_owned_elements.size() << std::endl;
    
    auto it = uilo_owned_elements.begin();
    size_t deletedCount = 0;
    
    while (it != uilo_owned_elements.end()) {
        Element* element = it->get();
        
        if (element->m_markedForDeletion) {
            std::cout << "  Deleting marked element: " << element->m_name << std::endl;
            
            if (!element->m_name.empty()) {
                auto buttonIt = buttons.find(element->m_name);
                if (buttonIt != buttons.end() && buttonIt->second == element) {
                    buttons.erase(buttonIt);
                }
                
                auto sliderIt = sliders.find(element->m_name);
                if (sliderIt != sliders.end() && sliderIt->second == element) {
                    sliders.erase(sliderIt);
                }
                
                auto textIt = texts.find(element->m_name);
                if (textIt != texts.end() && textIt->second == element) {
                    texts.erase(textIt);
                }
                
                auto spacerIt = spacers.find(element->m_name);
                if (spacerIt != spacers.end() && spacerIt->second == element) {
                    spacers.erase(spacerIt);
                }
                
                auto containerIt = containers.find(element->m_name);
                if (containerIt != containers.end() && containerIt->second == element) {
                    containers.erase(containerIt);
                }
            }
            
            it = uilo_owned_elements.erase(it);
            deletedCount++;
        } else {
            ++it;
        }
    }
    
    std::cout << "Cleaned up " << deletedCount << " marked elements" << std::endl;
    std::cout << "uilo_owned_elements size after: " << uilo_owned_elements.size() << std::endl;
}



// ---------------------------------------------------------------------------- //
// Row Implementation
// ---------------------------------------------------------------------------- //
inline void Row::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    applyModifiers();

    std::vector<Element*> left, center, right;
    for (auto& e : m_elements) {
        if (e->m_modifier.isVisible()) {
            Align a = e->m_modifier.getAlignment();
            if (hasAlign(a, Align::RIGHT)) right.push_back(e);
            else if (hasAlign(a, Align::CENTER_X)) center.push_back(e);
            else left.push_back(e); // default is LEFT
        }
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
    // Dirty check for Row itself
    m_isDirty = (m_bounds.getPosition() != m_pastBounds.getPosition() || m_bounds.getSize() != m_pastBounds.getSize());
    m_pastBounds.setPosition(m_bounds.getPosition());
    m_pastBounds.setSize(m_bounds.getSize());
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
        if (e->m_modifier.isVisible()) {
            Align a = e->m_modifier.getAlignment();
            if (hasAlign(a, Align::BOTTOM)) bottom.push_back(e);
            else if (hasAlign(a, Align::CENTER_Y)) center.push_back(e);
            else top.push_back(e); // default is TOP
        }
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
    // Dirty check for Column itself
    m_isDirty = (m_bounds.getPosition() != m_pastBounds.getPosition() || m_bounds.getSize() != m_pastBounds.getSize());
    m_pastBounds.setPosition(m_bounds.getPosition());
    m_pastBounds.setSize(m_bounds.getSize());
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
inline Text::Text(Modifier modifier, const std::string& str, sf::Font font, const std::string& name) 
: m_string(str), m_font(font){
    m_modifier = modifier;
    m_name = name;
    if (!m_name.empty()) {
        texts[m_name] = this;
    }
}

inline Text::Text(Modifier modifier, const std::string& str, const std::string& fontPath, const std::string& name)
: m_string(str) {
    m_modifier = modifier;

    if (!fontPath.empty())
        if (!m_font.openFromFile(fontPath))
            std::cerr << "Text Error: couldn't load font \"" << fontPath << "\".\n";
}

inline void Text::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    m_bounds.setPosition(parentBounds.getPosition());
    float fontSize = m_modifier.getFixedHeight() > 0
        ? m_modifier.getFixedHeight()
        : m_bounds.getSize().y;
    m_text.emplace(m_font, m_string);
    m_text->setCharacterSize(static_cast<unsigned>(fontSize));
    m_text->setFillColor(m_modifier.getColor());
    sf::FloatRect textBounds = m_text->getLocalBounds();
    m_bounds.setSize({ textBounds.size.x + textBounds.position.x, m_bounds.getSize().y });
    // Dirty check for Text
    m_isDirty = (m_bounds.getPosition() != m_pastBounds.getPosition() || m_bounds.getSize() != m_pastBounds.getSize());
    m_pastBounds.setPosition(m_bounds.getPosition());
    m_pastBounds.setSize(m_bounds.getSize());
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
    m_string = newStr;
    if (m_text)
        m_text->setString(newStr);
}



// ---------------------------------------------------------------------------- //
// Spacer Implementation
// ---------------------------------------------------------------------------- //
inline Spacer::Spacer(Modifier& modifier, const std::string& name) { 
    m_modifier = modifier;
    m_name = name;
    if (!m_name.empty()) {
        spacers[m_name] = this;
    }
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
    sf::Color textColor,
    const std::string& name
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

    m_name = name;
    if (!m_name.empty()) {
        buttons[m_name] = this;
    }
}

inline void Button::update(sf::RectangleShape& parentBounds) {
    applyModifiers();
    m_bounds.setSize({
        m_modifier.getFixedWidth() ? m_modifier.getFixedWidth() : parentBounds.getSize().x * m_modifier.getWidth(),
        m_modifier.getFixedHeight() ? m_modifier.getFixedHeight() : parentBounds.getSize().y * m_modifier.getHeight()
    });

    if (m_text) {
        m_text->update(m_bounds);
        m_text->m_bounds.setPosition({
            m_bounds.getPosition().x + (m_bounds.getSize().x / 2.f) - (m_text->m_bounds.getSize().x / 2.f),
            m_bounds.getPosition().y + (m_bounds.getSize().y / 2.f) - (m_text->m_bounds.getSize().y / 2.f)
        });
    }
    // Dirty check for Button
    m_isDirty = (m_bounds.getPosition() != m_pastBounds.getPosition() || m_bounds.getSize() != m_pastBounds.getSize());
    m_pastBounds.setPosition(m_bounds.getPosition());
    m_pastBounds.setSize(m_bounds.getSize());
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

    m_isClicked = false;
}

inline void Button::checkClick(const sf::Vector2f& pos) {
    if (m_bounds.getGlobalBounds().contains(pos)) {
        if (m_modifier.getOnClick()) m_modifier.getOnClick()();
        m_isClicked = true;
    }
}


inline void Button::setText(const std::string& newStr) {
    if (m_text)
        m_text->setString(newStr);
}

inline std::string Button::getText() const {
    if (m_text)
        return m_text->getString();
    return "";
}



// ---------------------------------------------------------------------------- //
// Slider Implementation
// ---------------------------------------------------------------------------- //
inline Slider::Slider(
    Modifier modifier,
    sf::Color knobColor,
    sf::Color barColor,
    const std::string& name
) : m_knobColor(knobColor), m_barColor(barColor) {
    m_modifier = modifier;
    m_knobRect.setFillColor(m_knobColor);
    m_barRect.setFillColor(m_barColor);

    m_name = name;
    if (!m_name.empty()) {
        sliders[m_name] = this;
    }
}

inline void Slider::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    applyModifiers();
    m_knobRect.setSize
    ({
        m_bounds.getSize().x, 
        m_bounds.getSize().x * 0.25f
    });
    
    m_barRect.setSize
    ({
        4.f, 
        m_bounds.getSize().y
    });

    m_barRect.setPosition
    ({
        m_bounds.getPosition().x + (m_bounds.getSize().x / 2) - 2, 
        m_bounds.getPosition().y
    });

    m_knobRect.setPosition
    ({
        m_bounds.getPosition().x, 
        m_bounds.getPosition().y + m_bounds.getSize().y - (m_bounds.getSize().y * m_curVal)
    });
    // Dirty check for Slider
    m_isDirty = (m_bounds.getPosition() != m_pastBounds.getPosition() || m_bounds.getSize() != m_pastBounds.getSize());
    m_pastBounds.setPosition(m_bounds.getPosition());
    m_pastBounds.setSize(m_bounds.getSize());
}

inline void Slider::render(sf::RenderTarget& target) {
    target.draw(m_barRect);
    target.draw(m_knobRect);
}

inline void Slider::checkClick(const sf::Vector2f& pos) {
    if (m_bounds.getGlobalBounds().contains(pos)) {
        float relY = pos.y - m_bounds.getPosition().y;
        float t = 1.f - (relY / m_bounds.getSize().y); // 1 at top, 0 at bottom
        float v = m_minVal + t * (m_maxVal - m_minVal);
        if (v < m_minVal) v = m_minVal;
        if (v > m_maxVal) v = m_maxVal;
        m_curVal = v;
    }
}

inline float Slider::getValue() const {
    return m_curVal;
}

inline void Slider::setValue(float newVal) {
    m_curVal = newVal < m_minVal ? m_minVal : (newVal > m_maxVal ? m_maxVal : newVal);
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

inline Row* row(
    Modifier modifier = default_mod, 
    std::initializer_list<Element*> elements = {}, 
    const std::string& name = ""
) { return obj<Row>(modifier, elements, name); }

inline Column* column(
    Modifier modifier = default_mod, 
    std::initializer_list<Element*> elements = {}, 
    const std::string& name = ""
) { return obj<Column>(modifier, elements, name); }

inline Spacer* spacer(
    Modifier modifier = default_mod, 
    const std::string& name = ""
) { return obj<Spacer>(modifier, name); }

inline Button* button(
    Modifier modifier = default_mod,
    ButtonStyle style = ButtonStyle::Default,
    const std::string& text = "",
    const std::string& fontPath = "",
    sf::Color textColor = sf::Color::White,
    const std::string& name = ""
) { return obj<Button>(modifier, style, text, fontPath, textColor, name); }

inline Text* text(
    Modifier modifier = default_mod,
    const std::string& str = "",
    const std::string& fontPath = "",
    const std::string& name = ""
) { return obj<Text>(modifier, str, fontPath, name); }

inline Slider* slider(
    Modifier modifier = default_mod,
    sf::Color knobColor = sf::Color::White,
    sf::Color barColor = sf::Color::Black,
    const std::string& name = ""
) { return obj<Slider>(modifier, knobColor, barColor, name); }

static Row* default_row = row();
static Column* default_column = column();
static Spacer* default_spacer = spacer();
static Button* default_button = button();
static Text* default_text = text();
static Slider* default_slider = slider();



// ---------------------------------------------------------------------------- //
// Global Element Getters
// ---------------------------------------------------------------------------- //
inline Row* getRow(const std::string& name) {
    if (containers.find(name) != containers.end()) {
        return dynamic_cast<Row*>(containers[name]);
    } else {
        std::cerr << "[UILO] Error: Row element \"" << name << "\" not found.\n";
        return default_row;
    }
}

inline Column* getColumn(const std::string& name) {
    if (containers.find(name) != containers.end()) {
        return dynamic_cast<Column*>(containers[name]);
    } else {
        std::cerr << "[UILO] Error: Column element \"" << name << "\" not found.\n";
        return default_column;
    }
}

inline Spacer* getSpacer(const std::string& name) {
    if (spacers.find(name) != spacers.end()) {
        return spacers[name];
    } else {
        std::cerr << "[UILO] Error: Spacer element \"" << name << "\" not found.\n";
        return default_spacer;
    }
}

inline Button* getButton(const std::string& name) {
    if (buttons.find(name) != buttons.end()) {
        return buttons[name];
    } else {
        std::cerr << "[UILO] Error: Button element \"" << name << "\" not found.\n";
        return default_button;
    }
}

inline Text* getText(const std::string& name) {
    if (texts.find(name) != texts.end()) {
        return texts[name];
    } else {
        std::cerr << "[UILO] Error: Text element \"" << name << "\" not found.\n";
        return default_text;
    }
}

inline Slider* getSlider(const std::string& name) {
    if (sliders.find(name) != sliders.end()) {
        return sliders[name];
    } else {
        std::cerr << "[UILO] Error: Slider element \"" << name << "\" not found.\n";
        return default_slider;
    }
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
        if (c->m_modifier.isVisible()) {
            c->m_bounds.setPosition(m_bounds.getPosition());
            c->update(m_bounds);
        }
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

inline void Page::clear() {
    for (auto& c : m_containers) {
        c->clear();
        c->m_markedForDeletion = true;
    }
    m_containers.clear();
    cleanupMarkedElements();
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

    for (auto& [name, btn] : buttons) {
        btn->setClicked(false);
    }

    if (!m_windowOwned)
        return;

    sf::Vector2u currentSize = m_window.getSize();
    if (currentSize != m_lastWindowSize || m_clickPosition) {
        m_shouldUpdate = true;
        m_pollCount = uilo_owned_elements.size();
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

    // if (m_mouseDragging && m_pollCount == 1) m_currentPage->dispatchClick(*m_clickPosition);
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

inline void UILO::addPage(std::pair<Page*, std::string> newPage) {
    Page*& page = newPage.first;
    const std::string& name = newPage.second;

    if (uilo_owned_pages.find(page) == uilo_owned_pages.end()) {
        uilo_owned_pages.insert(page);
    }
    m_ownedPages.push_back(std::unique_ptr<Page>(page));
    m_pages[name] = page;
    if (!m_currentPage) m_currentPage = page;
}

inline void UILO::addPages(std::initializer_list<std::pair<Page*, std::string>> pages) {
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

inline void UILO::forceUpdate() {
    m_shouldUpdate = true;
    m_pollCount = uilo_owned_elements.size();
}

inline void UILO::setScale(float scale) {
    m_renderScale = scale;
    initDefaultView();
    if (m_windowOwned) {
        m_window.setView(m_defaultView);
    } else if (m_userWindow) {
        m_userWindow->setView(m_defaultView);
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
                m_mouseDragging = true;
            }

            m_shouldUpdate = true;
        }

        if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
            m_mouseDragging = false;
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
    m_defScreenRes.size.x /= m_renderScale;
    m_defScreenRes.size.y /= m_renderScale;

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
