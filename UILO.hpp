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
#include <optional>

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
    ScrollableRow,
    Column,
    ScrollableColumn,
    FreeColumn,
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
    Modifier& onLClick(funcPtr cb);
    Modifier& onRClick(funcPtr cb);
    Modifier& setVisible(bool visible);
    Modifier& setHighPriority(bool highPriority);

    float getWidth() const;
    float getHeight() const;
    float getFixedWidth() const;
    float getFixedHeight() const;
    Align getAlignment() const;
    sf::Color getColor() const;
    const funcPtr& getOnLClick() const;
    const funcPtr& getOnRClick() const;
    bool isVisible() const;
    bool isHighPriority() const;

private:
    float m_widthPct = 1.f;
    float m_heightPct = 1.f;
    float m_fixedWidth = 0.f;
    float m_fixedHeight = 0.f;
    bool m_isVisible = true;
    bool m_highPriority = false;
    Align m_alignment = Align::NONE;
    sf::Color m_color = sf::Color::Transparent;
    std::function<void()> m_onLClick = nullptr;
    std::function<void()> m_onRClick = nullptr;
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
    bool m_isDirty = true;
    bool m_markedForDeletion = false;
    bool m_doRender = true;
    bool m_isHovered = false;
    std::vector<std::shared_ptr<sf::Drawable>> m_customGeometry;

    std::string m_name = "";

    Element();
    virtual ~Element();

    virtual void update(sf::RectangleShape& parentBounds);
    virtual void render(sf::RenderTarget& target);
    virtual void handleEvent(const sf::Event& event);
    virtual void checkClick(const sf::Vector2f& pos, sf::Mouse::Button button);
    virtual void checkHover(const sf::Vector2f& pos);
    virtual void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) {}
    void setModifier(const Modifier& modifier);
    virtual EType getType() const;
    sf::Vector2f getPosition() const { return m_bounds.getPosition(); }
    sf::Vector2f getSize() const { return m_bounds.getSize(); }
    bool isHovered() const { return m_isHovered; }
    void setCustomGeometry(std::vector<std::shared_ptr<sf::Drawable>>& customGeometry);

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
    virtual void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override {};
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
    void checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
    void checkHover(const sf::Vector2f& pos) override;
    virtual EType getType() const override;
    virtual void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override
    { for (auto& e : m_elements) e->checkScroll(pos, verticalDelta, horizontalDelta); }

private:
    inline void applyVerticalAlignment(Element* e, const sf::RectangleShape& parentBounds);
};

// ---------------------------------------------------------------------------- //
// Scrollable Row Container
// ---------------------------------------------------------------------------- //
class ScrollableRow : public Row {
public:
    using Row::Row;
    using Row::render;
    using Row::checkClick;
    using Row::checkHover;

    void update(sf::RectangleShape& parentBounds) override;
    void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override;
    virtual EType getType() const override;

    void setScrollSpeed(float speed) { m_scrollSpeed = speed; }
    void setOffset(float offset) { m_offset = offset; }
    float getOffset() const { return m_offset; }
    float getScrollSpeed() const { return m_scrollSpeed; }

private:
    float m_offset = 0.f;
    float m_scrollSpeed = 10.f;
};

// ---------------------------------------------------------------------------- //
// Column Container
// ---------------------------------------------------------------------------- //
class Column : public Container {
public:
    using Container::Container;

    void update(sf::RectangleShape& parentBounds) override;
    void render(sf::RenderTarget& target) override;
    void checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
    void checkHover(const sf::Vector2f& pos) override;
    virtual EType getType() const override;
    virtual void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override
    { for (auto& e : m_elements) e->checkScroll(pos, verticalDelta, horizontalDelta); }

private:
    inline void applyHorizontalAlignment(Element* e, const sf::RectangleShape& parentBounds);
};

// ---------------------------------------------------------------------------- //
// Scrollable Column Container
// ---------------------------------------------------------------------------- //
class ScrollableColumn : public Column {
public:
    using Column::Column;
    using Column::render;
    using Column::checkClick;
    using Column::checkHover;

    void update(sf::RectangleShape& parentBounds) override;
    void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override;
    virtual EType getType() const override;

    void setScrollSpeed(float speed) { m_scrollSpeed = speed; }
    void setOffset(float offset) { m_offset = offset; }
    float getOffset() const { return m_offset; }
    float getScrollSpeed() const { return m_scrollSpeed; }

private:
    float m_offset = 0.f;
    float m_scrollSpeed = 10.f;
};

// ---------------------------------------------------------------------------- //
// Free Column Container
// ---------------------------------------------------------------------------- //
class FreeColumn : public Column {
public:
    sf::Vector2f m_customPosition;

    using Column::Column;
    using Column::render;
    using Column::checkClick;
    using Column::checkHover;

    void update(sf::RectangleShape& parentBounds) override;
    void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override {}
    virtual EType getType() const override;
    void setPosition(sf::Vector2f pos);
    sf::Vector2f getPosition() const;
    sf::Vector2f getSize() const;
    sf::Vector2f getCenter() const;
    sf::FloatRect getBounds() const;
    void show() { m_modifier.setVisible(true); }
    void hide() { m_modifier.setVisible(false); }
};

// ---------------------------------------------------------------------------- //
// Text Element
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
    void checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
    void checkHover(const sf::Vector2f& pos) override;

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
    void checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;

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
    void dispatchClick(const sf::Vector2f& pos, sf::Mouse::Button button);
    void dispatchScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta);
    void dispatchHover(const sf::Vector2f& pos);
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
    bool windowShouldUpdate() const;
    void addPage(std::pair<Page*, std::string> newPage);
    void addPages(std::initializer_list<std::pair<Page*, std::string>> pages);
    void switchToPage(const std::string& pageName);
    void forceUpdate();
    void forceUpdate(sf::View& windowView);
    void setScale(float scale = 1.5f);
    sf::Vector2f getMousePosition() const;
    float getVerticalScrollDelta() const;
    void resetScrollDeltas();
    
    void setInputBlocked(bool blocked);
    bool isInputBlocked() const;

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

    bool m_running = false;
    bool m_shouldUpdate = true;
    bool m_mouseDragging = false;
    bool m_inputBlocked = false;

    sf::Vector2u m_lastWindowSize;
    std::optional<std::pair<sf::Vector2f, sf::Mouse::Button>> m_clickInfo;
    std::optional<sf::Vector2f> m_scrollPosition;
    sf::Vector2f m_mousePos;
    float m_verticalScrollDelta = 0.f;
    float m_horizontalScrollDelta = 0.f;

    std::vector<std::unique_ptr<Page>> m_ownedPages;

    std::vector<sf::RectangleShape> m_elementBoundsCache;
    bool m_cacheIsInitialized = false;

    void pollEvents();
    void initDefaultView();
    void setView(const sf::View& view);
    void _internal_update(sf::RenderWindow& target, sf::View& view);
};

// ---------------------------------------------------------------------------- //
// Modifier Implementation
// ---------------------------------------------------------------------------- //
inline Modifier& Modifier::setWidth(float pct) { m_widthPct = pct; m_fixedWidth = 0.f; return *this; }
inline Modifier& Modifier::setHeight(float pct) { m_heightPct = pct; m_fixedHeight = 0.f; return *this; }
inline Modifier& Modifier::setfixedWidth(float px) { m_fixedWidth = px; return *this; }
inline Modifier& Modifier::setfixedHeight(float px) { m_fixedHeight = px; return *this; }
inline Modifier& Modifier::align(Align alignment) { m_alignment = alignment; return *this; }
inline Modifier& Modifier::setColor(sf::Color color) { m_color = color; return *this; }
inline Modifier& Modifier::onLClick(funcPtr cb) { m_onLClick = std::move(cb); return *this; }
inline Modifier& Modifier::onRClick(funcPtr cb) { m_onRClick = std::move(cb); return *this; }
inline Modifier& Modifier::setVisible(bool visible) { m_isVisible = visible; return *this; }
inline Modifier& Modifier::setHighPriority(bool highPriority) { m_highPriority = highPriority; return *this; }

inline float Modifier::getWidth() const { return m_widthPct; }
inline float Modifier::getHeight() const { return m_heightPct; }
inline float Modifier::getFixedWidth() const { return m_fixedWidth; }
inline float Modifier::getFixedHeight() const { return m_fixedHeight; }
inline Align Modifier::getAlignment() const { return m_alignment; }
inline sf::Color Modifier::getColor() const { return m_color; }
inline const funcPtr& Modifier::getOnLClick() const { return m_onLClick; }
inline const funcPtr& Modifier::getOnRClick() const { return m_onRClick; }
inline bool Modifier::isVisible() const { return m_isVisible; }
inline bool Modifier::isHighPriority() const { return m_highPriority; }

// ---------------------------------------------------------------------------- //
// Element Implementation
// ---------------------------------------------------------------------------- //
inline Element::Element() {}

inline Element::~Element() {}

inline void Element::update(sf::RectangleShape& parentBounds) {
    m_isDirty = (m_bounds.getPosition() != m_pastBounds.getPosition() || m_bounds.getSize() != m_pastBounds.getSize());
    if (m_isDirty) {
        m_pastBounds.setPosition(m_bounds.getPosition());
        m_pastBounds.setSize(m_bounds.getSize());
    }
}

inline void Element::render(sf::RenderTarget& target) {}

inline void Element::handleEvent(const sf::Event& event) {
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        sf::Vector2f mousePos(mousePressed->position);
        if (m_bounds.getGlobalBounds().contains(mousePos)) {
            if (mousePressed->button == sf::Mouse::Button::Left) {
                if (m_modifier.getOnLClick()) m_modifier.getOnLClick()();
            } else if (mousePressed->button == sf::Mouse::Button::Right) {
                if (m_modifier.getOnRClick()) m_modifier.getOnRClick()();
            }
        }
    }
}

inline void Element::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
    if (m_bounds.getGlobalBounds().contains(pos)) {
        if (button == sf::Mouse::Button::Left) {
            if (m_modifier.getOnLClick()) m_modifier.getOnLClick()();
        } else if (button == sf::Mouse::Button::Right) {
            if (m_modifier.getOnRClick()) m_modifier.getOnRClick()();
        }
    }
}

inline void Element::checkHover(const sf::Vector2f& pos) {
    m_isHovered = m_bounds.getGlobalBounds().contains(pos);
}

inline void Element::setModifier(const Modifier& modifier) { m_modifier = modifier; }

inline EType Element::getType() const { return EType::Element; }

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

inline void Element::applyModifiers() { m_bounds.setFillColor(m_modifier.getColor()); }

inline void Element::setCustomGeometry(std::vector<std::shared_ptr<sf::Drawable>>& customGeometry) {
    m_customGeometry.clear();
    m_customGeometry = customGeometry;
}

// ---------------------------------------------------------------------------- //
// Container Implementation
// ---------------------------------------------------------------------------- //
inline Container::Container(std::initializer_list<Element*> elements, const std::string& name) {
    for (auto& e : elements)
        m_elements.push_back(e);

    m_name = name;

    if (!m_name.empty())
        containers[m_name] = this;
}

inline Container::Container(Modifier modifier, std::initializer_list<Element*> elements, const std::string& name) {
    m_modifier = modifier;

    for (auto& e : elements)
        m_elements.push_back(e);

    m_name = name;

    if (!m_name.empty())
        containers[m_name] = this;
}

inline Container::~Container() {}

inline void Container::addElement(Element* element) { m_elements.push_back(element); }

inline void Container::addElements(std::initializer_list<Element*> elements) {
    for (auto& e : elements)
        m_elements.push_back(e);
}

inline void Container::handleEvent(const sf::Event& event) {
    for (auto& e : m_elements)
        e->handleEvent(event);
    Element::handleEvent(event);
}

inline const std::vector<Element*>& Container::getElements() const { return m_elements; }

inline void Container::clear() {
    for (auto& e : m_elements) {
        if (Container* childContainer = dynamic_cast<Container*>(e)) {
            childContainer->clear();
        }
        e->m_markedForDeletion = true;
    }

    m_elements.clear();
    cleanupMarkedElements();
}

inline void cleanupMarkedElements() {
    auto it = uilo_owned_elements.begin();
    while (it != uilo_owned_elements.end()) {
        Element* element = it->get();
        if (element->m_markedForDeletion) {
            if (!element->m_name.empty()) {
                auto buttonIt = buttons.find(element->m_name);
                if (buttonIt != buttons.end() && buttonIt->second == element)
                    buttons.erase(buttonIt);

                auto sliderIt = sliders.find(element->m_name);
                if (sliderIt != sliders.end() && sliderIt->second == element)
                    sliders.erase(sliderIt);

                auto textIt = texts.find(element->m_name);
                if (textIt != texts.end() && textIt->second == element)
                    texts.erase(textIt);

                auto spacerIt = spacers.find(element->m_name);
                if (spacerIt != spacers.end() && spacerIt->second == element)
                    spacers.erase(spacerIt);

                auto containerIt = containers.find(element->m_name);
                if (containerIt != containers.end() && containerIt->second == element)
                    containers.erase(containerIt);
            }
            it = uilo_owned_elements.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------- //
// Row Implementation
// ---------------------------------------------------------------------------- //
inline void Row::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    applyModifiers();
    Element::update(parentBounds);

    float totalPercent = 0.f, totalFixed = 0.f;
    for (auto& e : m_elements) if (e->m_modifier.isVisible()) {
        if (e->m_modifier.getFixedWidth() > 0.f) totalFixed += e->m_modifier.getFixedWidth();
        else totalPercent += e->m_modifier.getWidth();
    }
    float remainingSpace = m_bounds.getSize().x - totalFixed;
    if (totalPercent <= 0) totalPercent = 1.f;

    for (auto& e : m_elements) if (e->m_modifier.isVisible()) {
        float width = e->m_modifier.getFixedWidth() > 0.f ? e->m_modifier.getFixedWidth() : (e->m_modifier.getWidth() / totalPercent) * remainingSpace;
        sf::RectangleShape slot({width, m_bounds.getSize().y});
        e->update(slot);
    }

    std::vector<Element*> left, center, right;
    float leftWidth = 0, centerWidth = 0, rightWidth = 0;

    for (auto& e : m_elements) {
        if (e->m_modifier.isVisible()) {
            Align a = e->m_modifier.getAlignment();
            if (hasAlign(a, Align::RIGHT)) {
                right.push_back(e);
                rightWidth += e->m_bounds.getSize().x;
            }
            else if (hasAlign(a, Align::CENTER_X)) {
                center.push_back(e);
                centerWidth += e->m_bounds.getSize().x;
            }
            else {
                left.push_back(e);
                leftWidth += e->m_bounds.getSize().x;
            }
        }
    }

    float xPos;
    xPos = m_bounds.getPosition().x;
    for (auto& e : left) {
        e->m_bounds.setPosition({xPos, e->m_bounds.getPosition().y});
        xPos += e->m_bounds.getSize().x;
    }

    xPos = m_bounds.getPosition().x + (m_bounds.getSize().x - centerWidth) / 2.f;
    for (auto& e : center) {
        e->m_bounds.setPosition({xPos, e->m_bounds.getPosition().y});
        xPos += e->m_bounds.getSize().x;
    }

    xPos = m_bounds.getPosition().x + m_bounds.getSize().x - rightWidth;
    for (auto& e : right) {
        e->m_bounds.setPosition({xPos, e->m_bounds.getPosition().y});
        xPos += e->m_bounds.getSize().x;
    }

    for (auto& e : m_elements) if (e->m_modifier.isVisible()) {
        sf::Vector2f pos = e->m_bounds.getPosition();
        Align a = e->m_modifier.getAlignment();

        if (hasAlign(a, Align::CENTER_Y))
            pos.y = m_bounds.getPosition().y + (m_bounds.getSize().y - e->m_bounds.getSize().y) / 2.f;
        else if (hasAlign(a, Align::BOTTOM))
            pos.y = m_bounds.getPosition().y + m_bounds.getSize().y - e->m_bounds.getSize().y;
        else
            pos.y = m_bounds.getPosition().y;

        e->m_bounds.setPosition(pos);
    }
}

inline void Row::render(sf::RenderTarget& target) {
    if (getType() == EType::ScrollableRow) {
        sf::View originalView = target.getView();
        sf::FloatRect clipRect = m_bounds.getGlobalBounds();
        sf::View clippingView(clipRect);

        sf::Vector2f worldPos = {clipRect.position.x, clipRect.position.y};
        sf::Vector2i pixelPos = target.mapCoordsToPixel(worldPos, originalView);

        sf::Vector2u windowSize = target.getSize();
        sf::FloatRect viewport(
            {static_cast<float>(pixelPos.x) / windowSize.x,
             static_cast<float>(pixelPos.y) / windowSize.y},
            {clipRect.size.x / windowSize.x,
             clipRect.size.y / windowSize.y}
        );

        clippingView.setViewport(viewport);
        target.setView(clippingView);

        target.draw(m_bounds);

        sf::RenderStates states;
        states.transform.translate(m_bounds.getPosition());
        
        // Render normal priority elements first
        for (auto& e : m_elements)
            if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority())
                e->render(target);
        
        for (auto& d : m_customGeometry) {
            target.draw(*d, states);
        }
        
        // Then render high priority elements on top
        for (auto& e : m_elements)
            if (e->m_modifier.isVisible() && e->m_doRender && e->m_modifier.isHighPriority())
                e->render(target);

        target.setView(originalView);
    }
    else {
        target.draw(m_bounds);
        
        // Render normal priority elements first
        for (auto& e : m_elements)
            if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority())
                e->render(target);
        
        // Then render high priority elements on top
        for (auto& e : m_elements)
            if (e->m_modifier.isVisible() && e->m_doRender && e->m_modifier.isHighPriority())
                e->render(target);
    }
}

inline void Row::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
    for (auto& e : m_elements)
        if (e) e->checkClick(pos, button);
    Element::checkClick(pos, button);
}

inline void Row::checkHover(const sf::Vector2f& pos) {
    Element::checkHover(pos);
    for (auto& e : m_elements)
        if (e) e->checkHover(pos);
}

inline EType Row::getType() const { return EType::Row; }

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
// Scrollable Row Implementation
// ---------------------------------------------------------------------------- //
inline void ScrollableRow::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    applyModifiers();
    Element::update(parentBounds);

    Row::update(parentBounds);

    if (!m_elements.empty()) {
        for (auto& e : m_elements) {
            if (e->m_modifier.isVisible())
                e->m_bounds.setPosition({ e->m_bounds.getPosition().x + m_offset, e->m_bounds.getPosition().y});
        }

        if (m_elements.back()->m_bounds.getPosition().x <= m_bounds.getPosition().x)
            m_offset += m_bounds.getPosition().x - m_elements.back()->m_bounds.getPosition().x;
        else if (m_elements.front()->m_bounds.getPosition().x >= m_bounds.getPosition().x)
            m_offset -= m_elements.front()->m_bounds.getPosition().x - m_bounds.getPosition().x;
    }
}

inline void ScrollableRow::checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) {
    if (m_bounds.getGlobalBounds().contains(pos)) {
        if (horizontalDelta < 0)
            m_offset -= m_scrollSpeed;
        else if (horizontalDelta > 0)
            m_offset += m_scrollSpeed;
        else if (verticalDelta != 0)
            for (auto& e : m_elements)
                e->checkScroll(pos, verticalDelta, horizontalDelta);
    }
}

inline EType ScrollableRow::getType() const {
    return EType::ScrollableRow;
}

// ---------------------------------------------------------------------------- //
// Column Implementation
// ---------------------------------------------------------------------------- //
inline void Column::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    applyModifiers();
    Element::update(parentBounds);

    float totalPercent = 0.f, totalFixed = 0.f;
    for (auto& e : m_elements) if (e->m_modifier.isVisible()) {
        if (e->m_modifier.getFixedHeight() > 0.f) totalFixed += e->m_modifier.getFixedHeight();
        else totalPercent += e->m_modifier.getHeight();
    }

    float remainingSpace = m_bounds.getSize().y - totalFixed;
    if (totalPercent <= 0) totalPercent = 1.f;

    for (auto& e : m_elements) if (e->m_modifier.isVisible()) {
        float height = e->m_modifier.getFixedHeight() > 0.f ? e->m_modifier.getFixedHeight() : (e->m_modifier.getHeight() / totalPercent) * remainingSpace;
        sf::RectangleShape slot({m_bounds.getSize().x, height});
        e->update(slot);
    }

    std::vector<Element*> top, center, bottom;
    float topHeight = 0, centerHeight = 0, bottomHeight = 0;

    for (auto& e : m_elements) {
        if (e->m_modifier.isVisible()) {
            Align a = e->m_modifier.getAlignment();

            if (hasAlign(a, Align::BOTTOM)) {
                bottom.push_back(e);
                bottomHeight += e->m_bounds.getSize().y;
            }
            else if (hasAlign(a, Align::CENTER_Y)) {
                center.push_back(e);
                centerHeight += e->m_bounds.getSize().y;
            }
            else {
                top.push_back(e);
                topHeight += e->m_bounds.getSize().y;
            }
        }
    }

    float yPos;

    yPos = m_bounds.getPosition().y;
    for (auto& e : top) {
        e->m_bounds.setPosition({e->m_bounds.getPosition().x, yPos});
        yPos += e->m_bounds.getSize().y;
    }

    yPos = m_bounds.getPosition().y + (m_bounds.getSize().y - centerHeight) / 2.f;
    for (auto& e : center) {
        e->m_bounds.setPosition({e->m_bounds.getPosition().x, yPos});
        yPos += e->m_bounds.getSize().y;
    }

    yPos = m_bounds.getPosition().y + m_bounds.getSize().y - bottomHeight;
    for (auto& e : bottom) {
        e->m_bounds.setPosition({e->m_bounds.getPosition().x, yPos});
        yPos += e->m_bounds.getSize().y;
    }

    for (auto& e : m_elements) if (e->m_modifier.isVisible()) {
        sf::Vector2f pos = e->m_bounds.getPosition();
        Align a = e->m_modifier.getAlignment();

        if (hasAlign(a, Align::CENTER_X))
            pos.x = m_bounds.getPosition().x + (m_bounds.getSize().x - e->m_bounds.getSize().x) / 2.f;
        else if (hasAlign(a, Align::RIGHT))
            pos.x = m_bounds.getPosition().x + m_bounds.getSize().x - e->m_bounds.getSize().x;
        else
            pos.x = m_bounds.getPosition().x;

        e->m_bounds.setPosition(pos);
    }
}

inline void Column::render(sf::RenderTarget& target) {
    if (getType() == EType::ScrollableColumn) {
        sf::View originalView = target.getView();
        sf::FloatRect clipRect = m_bounds.getGlobalBounds();
        sf::View clippingView(clipRect);

        sf::Vector2f worldPos = {clipRect.position.x, clipRect.position.y};
        sf::Vector2i pixelPos = target.mapCoordsToPixel(worldPos, originalView);

        sf::Vector2u windowSize = target.getSize();
        sf::FloatRect viewport(
            {static_cast<float>(pixelPos.x) / windowSize.x,
             static_cast<float>(pixelPos.y) / windowSize.y},
            {clipRect.size.x / windowSize.x,
             clipRect.size.y / windowSize.y}
        );

        clippingView.setViewport(viewport);
        target.setView(clippingView);

        target.draw(m_bounds);
        
        sf::RenderStates states;
        states.transform.translate(m_bounds.getPosition());

        // Render normal priority elements first
        for (auto& e : m_elements)
            if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority())
                e->render(target);


        for (auto& d : m_customGeometry) {
            target.draw(*d, states);
        }
        
        // Then render high priority elements on top
        for (auto& e : m_elements)
            if (e->m_modifier.isVisible() && e->m_doRender && e->m_modifier.isHighPriority())
                e->render(target);

        target.setView(originalView);
    }
    else {
        target.draw(m_bounds);
        
        // Render normal priority elements first
        for (auto& e : m_elements)
            if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority())
                e->render(target);
        
        // Then render high priority elements on top
        for (auto& e : m_elements)
            if (e->m_modifier.isVisible() && e->m_doRender && e->m_modifier.isHighPriority())
                e->render(target);
    }
}

inline void Column::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
    for (auto& e : m_elements)
        if (e) e->checkClick(pos, button);
    Element::checkClick(pos, button);
}

inline void Column::checkHover(const sf::Vector2f& pos) {
    Element::checkHover(pos);
    for (auto& e : m_elements)
        if (e) e->checkHover(pos);
}

inline EType Column::getType() const { return EType::Column; }

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
// Scrollable Column Implementation
// ---------------------------------------------------------------------------- //
inline void ScrollableColumn::update(sf::RectangleShape& parentBounds) {
    resize(parentBounds);
    applyModifiers();
    Element::update(parentBounds);

    Column::update(parentBounds);

    if (!m_elements.empty()) {
        for (auto& e : m_elements) {
            if (e->m_modifier.isVisible())
                e->m_bounds.setPosition({ e->m_bounds.getPosition().x, e->m_bounds.getPosition().y + m_offset});

            const std::optional<sf::FloatRect> intersection = m_bounds.getGlobalBounds().findIntersection(e->m_bounds.getGlobalBounds());
            e->m_doRender = intersection.has_value();
        }

        if (m_elements.back()->m_bounds.getPosition().y <= m_bounds.getPosition().y)
            m_offset += m_bounds.getPosition().y - m_elements.back()->m_bounds.getPosition().y;
        else if (m_elements.front()->m_bounds.getPosition().y >= m_bounds.getPosition().y)
            m_offset -= m_elements.front()->m_bounds.getPosition().y - m_bounds.getPosition().y;
    }
}

inline void ScrollableColumn::checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) {
    if (m_bounds.getGlobalBounds().contains(pos)) {
        if (verticalDelta < 0)
            m_offset -= m_scrollSpeed;
        else if (verticalDelta > 0)
            m_offset += m_scrollSpeed;
        else if (horizontalDelta != 0)
            for (auto& e : m_elements)
                e->checkScroll(pos, verticalDelta, horizontalDelta);
    }
}

inline EType ScrollableColumn::getType() const { return EType::ScrollableColumn; }

// ---------------------------------------------------------------------------- //
// Free Column Implementation
// ---------------------------------------------------------------------------- //
inline void FreeColumn::update(sf::RectangleShape& parentBounds) {
    sf::RectangleShape newParentBounds({0, 0});
    newParentBounds.setPosition(m_customPosition);
    newParentBounds.setSize(m_bounds.getSize());
    Column::update(newParentBounds);
    m_bounds.setPosition(m_customPosition);
    Element::update(parentBounds);
}

inline EType FreeColumn::getType() const { return EType::FreeColumn; }

inline void FreeColumn::setPosition(sf::Vector2f pos) { m_customPosition = pos; }

inline sf::Vector2f FreeColumn::getPosition() const { return m_customPosition; }

inline sf::Vector2f FreeColumn::getSize() const { return m_bounds.getSize(); }

inline sf::Vector2f FreeColumn::getCenter() const {
    return
    {
        m_customPosition.x + (m_bounds.getSize().x / 2),
        m_customPosition.y + (m_bounds.getSize().y / 2)
    };
}

inline sf::FloatRect FreeColumn::getBounds() const {
    return sf::FloatRect(m_customPosition, m_bounds.getSize());
}

// ---------------------------------------------------------------------------- //
// Text Implementation
// ---------------------------------------------------------------------------- //
inline Text::Text(Modifier modifier, const std::string& str, sf::Font font, const std::string& name)
: m_string(str), m_font(font){
    m_modifier = modifier;
    m_name = name;

    if (!m_name.empty())
        texts[m_name] = this;
}

inline Text::Text(Modifier modifier, const std::string& str, const std::string& fontPath, const std::string& name)
: m_string(str) {
    m_modifier = modifier;
    m_name = name;
    if (!m_name.empty())
        texts[m_name] = this;

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

    Element::update(parentBounds);
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
    m_isDirty = true;
}

// ---------------------------------------------------------------------------- //
// Spacer Implementation
// ---------------------------------------------------------------------------- //
inline Spacer::Spacer(Modifier& modifier, const std::string& name) {
    m_modifier = modifier;
    m_name = name;

    if (!m_name.empty())
        spacers[m_name] = this;
}

inline void Spacer::update(sf::RectangleShape& parentBounds) {
    m_bounds.setFillColor(sf::Color::Transparent);
    resize(parentBounds);
    applyModifiers();
    Element::update(parentBounds);
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

    Element::update(parentBounds);
}

inline void Button::render (sf::RenderTarget& target) {
    if (m_buttonStyle == ButtonStyle::Default || m_buttonStyle == ButtonStyle::Rect)
        target.draw(m_bounds);
    else {
        m_leftCircle.setPointCount(static_cast<unsigned int>(m_bounds.getSize().y * 2));
        m_rightCircle.setPointCount(static_cast<unsigned int>(m_bounds.getSize().y * 2));

        m_leftCircle.setRadius(m_bounds.getSize().y / 2);
        m_rightCircle.setRadius(m_bounds.getSize().y / 2);
        m_bodyRect.setSize(
        {
            m_bounds.getSize().x - m_bounds.getSize().y,
            m_bounds.getSize().y
        });
        m_leftCircle.setPosition(m_bounds.getPosition());
        m_rightCircle.setPosition(
        {
            m_bounds.getPosition().x + m_bounds.getSize().x - m_bounds.getSize().y,
            m_bounds.getPosition().y
        });
        m_bodyRect.setPosition(
        {
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

inline void Button::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
    if (m_bounds.getGlobalBounds().contains(pos)) {
        if (button == sf::Mouse::Button::Left) {
            m_isClicked = true;
            if (m_modifier.getOnLClick()) {
                m_modifier.getOnLClick()();
            }
        } else if (button == sf::Mouse::Button::Right) {
            if (m_modifier.getOnRClick()) {
                m_modifier.getOnRClick()();
            }
        }
    }
}

inline void Button::checkHover(const sf::Vector2f& pos) {
    m_isHovered = m_bounds.getGlobalBounds().contains(pos);
    Element::checkHover(pos);
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

    m_knobRect.setSize(
    {
        m_bounds.getSize().x,
        m_bounds.getSize().x * 0.25f
    });

    m_barRect.setSize(
    {
        4.f,
        m_bounds.getSize().y
    });

    m_barRect.setPosition(
    {
        m_bounds.getPosition().x + (m_bounds.getSize().x / 2) - 2,
        m_bounds.getPosition().y
    });

    m_knobRect.setPosition(
    {
        m_bounds.getPosition().x,
        m_bounds.getPosition().y + m_bounds.getSize().y - (m_bounds.getSize().y * m_curVal)
    });

    Element::update(parentBounds);
}

inline void Slider::render(sf::RenderTarget& target) {
    target.draw(m_barRect);
    target.draw(m_knobRect);
}

inline void Slider::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
    if (button == sf::Mouse::Button::Left && m_bounds.getGlobalBounds().contains(pos)) {
        float relY = pos.y - m_bounds.getPosition().y;
        float t = 1.f - (relY / m_bounds.getSize().y);
        float v = m_minVal + t * (m_maxVal - m_minVal);

        if (v < m_minVal) v = m_minVal;
        if (v > m_maxVal) v = m_maxVal;
        m_curVal = v;
    }
}

inline float Slider::getValue() const { return m_curVal; }

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

inline ScrollableRow* scrollableRow(
    Modifier modifier = default_mod,
    std::initializer_list<Element*> elements = {},
    const std::string& name = ""
) { return obj<ScrollableRow>(modifier, elements, name); }

inline Column* column(
    Modifier modifier = default_mod,
    std::initializer_list<Element*> elements = {},
    const std::string& name = ""
) { return obj<Column>(modifier, elements, name); }

inline ScrollableColumn* scrollableColumn(
    Modifier modifier = default_mod,
    std::initializer_list<Element*> elements = {},
    const std::string& name = ""
) { return obj<ScrollableColumn>(modifier, elements, name); }

inline FreeColumn* freeColumn(
    Modifier modifier = default_mod,
    std::initializer_list<Element*> elements = {},
    const std::string& name = ""
) { return obj<FreeColumn>(modifier, elements, name); }

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
static Spacer* default_spacer = new Spacer(default_mod);
static Button* default_button = button();
static Text* default_text = text();
static Slider* default_slider = slider();

// ---------------------------------------------------------------------------- //
// Global Element Getters
// ---------------------------------------------------------------------------- //
inline Row* getRow(const std::string& name) {
    if (containers.count(name))
        return dynamic_cast<Row*>(containers[name]);
    else {
        std::cerr << "[UILO] Error: Row element \"" << name << "\" not found.\n";
        return default_row;
    }
}

inline Column* getColumn(const std::string& name) {
    if (containers.count(name))
        return dynamic_cast<Column*>(containers[name]);
    else {
        std::cerr << "[UILO] Error: Column element \"" << name << "\" not found.\n";
        return default_column;
    }
}

inline Spacer* getSpacer(const std::string& name) {
    if (spacers.count(name))
        return spacers[name];
    else {
        std::cerr << "[UILO] Error: Spacer element \"" << name << "\" not found.\n";
        return default_spacer;
    }
}

inline Button* getButton(const std::string& name) {
    if (buttons.count(name))
        return buttons[name];
    else {
        std::cerr << "[UILO] Error: Button element \"" << name << "\" not found.\n";
        return default_button;
    }
}

inline Text* getText(const std::string& name) {
    if (texts.count(name))
        return texts[name];
    else {
        std::cerr << "[UILO] Error: Text element \"" << name << "\" not found.\n";
        return default_text;
    }
}

inline Slider* getSlider(const std::string& name) {
    if (sliders.count(name))
        return sliders[name];
    else {
        std::cerr << "[UILO] Error: Slider element \"" << name << "\" not found.\n";
        return default_slider;
    }
}

// ---------------------------------------------------------------------------- //
// Page Implementation
// ---------------------------------------------------------------------------- //
inline Page::Page(std::initializer_list<Container*> containers) {
    m_bounds.setFillColor(sf::Color::Transparent);

    for (const auto& c : containers)
        m_containers.push_back(c);

    uilo_owned_pages.insert(this);
}

inline Page::~Page() {
    if (uilo_owned_pages.count(this) && !time_to_delete) {
        std::cerr << "[UILO] Error: Attempted to delete a UILO-owned page directly.\n";
        std::abort();
    }
}

inline void Page::update(const sf::RectangleShape& parentBounds) {
    m_bounds = parentBounds;

    for (auto& c : m_containers) {
        if (c->m_modifier.isVisible()) {
            if (!(c->getType() == EType::FreeColumn))
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

inline void Page::dispatchClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
    for (auto& c : m_containers)
        c->checkClick(pos, button);
}

inline void Page::dispatchScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) {
    for (auto& c : m_containers)
        c->checkScroll(pos, verticalDelta, horizontalDelta);
}

inline void Page::dispatchHover(const sf::Vector2f& pos) {
    for (auto& c : m_containers)
        c->checkHover(pos);
}

inline void Page::clear() {
    for (auto& c : m_containers) {
        c->clear();
        c->m_markedForDeletion = true;
    }
    m_containers.clear();
    cleanupMarkedElements();
}

inline Page* page(std::initializer_list<Container*> containers = {}) { return new Page(containers); }

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

    addPages(pages);
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

    addPages(pages);
    m_bounds.setFillColor(sf::Color::Transparent);
}

inline UILO::~UILO() {
    time_to_delete = true;
    uilo_owned_elements.clear();
    uilo_owned_pages.clear();
    m_pages.clear();
}

inline void UILO::_internal_update(sf::RenderWindow& target, sf::View& view) {
    pollEvents();

    static bool firstFrame = true;
    if (!firstFrame) {
        for (auto& [name, btn] : uilo::buttons) {
            btn->setClicked(false);
        }
    }
    firstFrame = false;

    sf::Vector2u currentSize = target.getSize();
    if (currentSize != m_lastWindowSize) {
        m_shouldUpdate = true;
        m_lastWindowSize = currentSize;
    }

    if (!m_shouldUpdate && m_cacheIsInitialized && m_elementBoundsCache.size() == uilo_owned_elements.size()) {
        for (size_t i = 0; i < uilo_owned_elements.size(); ++i) {
            const auto& element = uilo_owned_elements[i];
            const auto& cachedBounds = m_elementBoundsCache[i];
            if (element->m_bounds.getPosition() != cachedBounds.getPosition() ||
                element->m_bounds.getSize() != cachedBounds.getSize() ||
                element->m_isDirty) {
                m_shouldUpdate = true;
                break;
            }
        }
    } else if (!m_cacheIsInitialized || m_elementBoundsCache.size() != uilo_owned_elements.size()) {
        m_shouldUpdate = true;
    }

    if (m_shouldUpdate) {
        view.setSize({ (float)currentSize.x, (float)currentSize.y });
        m_bounds.setSize(view.getSize());
        m_bounds.setPosition({
            view.getCenter().x - view.getSize().x * 0.5f,
            view.getCenter().y - view.getSize().y * 0.5f
        });
        target.setView(view);

        if (m_currentPage) {
            for (int i = 0; i < 12; ++i)
                m_currentPage->update(m_bounds);
        }

        m_elementBoundsCache.clear();
        m_elementBoundsCache.reserve(uilo_owned_elements.size());
        for (const auto& elementPtr : uilo_owned_elements) {
            m_elementBoundsCache.push_back(elementPtr->m_bounds);
            elementPtr->m_isDirty = false;
        }
        m_cacheIsInitialized = true;
    }

    if (m_clickInfo) {
        if (m_currentPage && !m_inputBlocked) {
            m_currentPage->dispatchClick(m_clickInfo->first, m_clickInfo->second);
        }
        m_clickInfo.reset();
    }

    if (m_scrollPosition) {
        if (m_currentPage && !m_inputBlocked) {
            m_currentPage->dispatchScroll(*m_scrollPosition, m_verticalScrollDelta, m_horizontalScrollDelta);
        }
        m_scrollPosition.reset();
    }
}

inline void UILO::update() {
    sf::RenderWindow& target = m_windowOwned ? m_window : *m_userWindow;
    
    // Update mouse position every frame using real-time mouse position
    m_mousePos = target.mapPixelToCoords(sf::Mouse::getPosition(target));
    
    _internal_update(target, m_defaultView);
    
    // Always check hover for the current mouse position on every frame
    if (m_currentPage && !m_inputBlocked) {
        m_currentPage->dispatchHover(m_mousePos);
    }
}

inline void UILO::update(sf::View& windowView) {
    if (m_windowOwned) return;
    
    // Update mouse position every frame using real-time mouse position
    m_mousePos = m_userWindow->mapPixelToCoords(sf::Mouse::getPosition(*m_userWindow));
    
    _internal_update(*m_userWindow, windowView);
    
    // Always check hover for the current mouse position on every frame
    if (m_currentPage && !m_inputBlocked) {
        m_currentPage->dispatchHover(m_mousePos);
    }
}

inline void UILO::render() {
    if (m_shouldUpdate) {
        sf::RenderWindow& target = m_windowOwned ? m_window : *m_userWindow;

        if (m_windowOwned) {
            target.clear(sf::Color::Black);
            if (m_currentPage) m_currentPage->render(target);
            target.display();
        } else {
            if (m_currentPage) m_currentPage->render(target);
        }
    }
    m_shouldUpdate = false;
}

inline void UILO::setTitle(const std::string& newTitle) {
    if (m_windowOwned) m_window.setTitle(newTitle);
}

inline bool UILO::isRunning() const { return m_running; }
inline bool UILO::windowShouldUpdate() const { return m_shouldUpdate; }

inline void UILO::addPage(std::pair<Page*, std::string> newPage) {
    Page*& page = newPage.first;
    const std::string& name = newPage.second;

    if (!uilo_owned_pages.count(page))
        uilo_owned_pages.insert(page);

    m_ownedPages.push_back(std::unique_ptr<Page>(page));
    m_pages[name] = page;
    if (!m_currentPage) m_currentPage = page;
    m_cacheIsInitialized = false;
}

inline void UILO::addPages(std::initializer_list<std::pair<Page*, std::string>> pages) {
    for (const auto& [page, name] : pages) {
        if (!uilo_owned_pages.count(page)) {
            uilo_owned_pages.insert(page);
        }
        m_ownedPages.push_back(std::unique_ptr<Page>(page));
        m_pages[name] = page;
        if (!m_currentPage) m_currentPage = page;
    }
    m_cacheIsInitialized = false;
}

inline void UILO::switchToPage(const std::string& pageName) {
    auto it = m_pages.find(pageName);
    if (it != m_pages.end()) {
        if (m_currentPage != it->second) {
            m_currentPage = it->second;
            m_shouldUpdate = true;
        }
    } else {
        std::cerr << "[UILO] Page \"" << pageName << "\" not found.\n";
    }
}

inline void UILO::forceUpdate() {
    m_shouldUpdate = true;
    update();
}

inline void UILO::forceUpdate(sf::View& windowView) {
    m_shouldUpdate = true;
    update(windowView);
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

inline sf::Vector2f UILO::getMousePosition() const { return m_mousePos; }
inline float UILO::getVerticalScrollDelta() const { return m_verticalScrollDelta; }
inline void UILO::resetScrollDeltas() { 
    m_verticalScrollDelta = 0.f; 
    m_horizontalScrollDelta = 0.f; 
}

inline void UILO::setInputBlocked(bool blocked) { m_inputBlocked = blocked; }
inline bool UILO::isInputBlocked() const { return m_inputBlocked; }

inline void UILO::pollEvents() {
    sf::RenderWindow* activeWindow = m_windowOwned ? &m_window : m_userWindow;
    if (!activeWindow) return;

    while (const auto event = activeWindow->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            activeWindow->close();
            m_running = false;
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Mouse::Button button = mousePressed->button;
            if (button == sf::Mouse::Button::Left || button == sf::Mouse::Button::Right) {
                m_clickInfo = { activeWindow->mapPixelToCoords(mousePressed->position), button };
                m_shouldUpdate = true;
                if (button == sf::Mouse::Button::Left) {
                    m_mouseDragging = true;
                }
            }
        }

        if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
            m_mousePos = activeWindow->mapPixelToCoords(mouseMoved->position);
        }

        if (const auto* mouseScrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
            m_scrollPosition = activeWindow->mapPixelToCoords(mouseScrolled->position);
            if (mouseScrolled->wheel == sf::Mouse::Wheel::Vertical) {
                m_verticalScrollDelta = mouseScrolled->delta;
                m_horizontalScrollDelta = 0.f;
            }
            else if (mouseScrolled->wheel == sf::Mouse::Wheel::Horizontal) {
                m_horizontalScrollDelta = mouseScrolled->delta;
                m_verticalScrollDelta = 0.f;
            }
            m_shouldUpdate = true;
        }

        if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
            m_mouseDragging = false;
        }
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

// -------------------scale--------------------------------------------------------- //
// Alignment Helpers
// ---------------------------------------------------------------------------- //
inline Align operator|(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline Align operator&(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool hasAlign(Align value, Align flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

using contains = std::initializer_list<uilo::Element*>;

} // !namespace uilo

#endif // !UILO_HPP