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

inline Align operator|(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline Align operator&(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool hasAlign(Align value, Align flag) {
    return static_cast<uint8_t>(value & flag) != 0;
}



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
// Global Ownership Sets
// ---------------------------------------------------------------------------- //
static std::unordered_set<Page*> uilo_owned_pages;
static std::unordered_set<Element*> uilo_owned_elements;
static bool time_to_delete = false;



// ---------------------------------------------------------------------------- //
// Modifier
// ---------------------------------------------------------------------------- //
class Modifier {
public:
    Modifier() = default;

    Modifier& setWidth(float pct)                   { m_widthPct = pct; m_fixedWidth = 0.f; return *this; }
    Modifier& setHeight(float pct)                  { m_heightPct = pct; m_fixedHeight = 0.f; return *this; }
    Modifier& setfixedWidth(float px)               { m_fixedWidth = px; return *this; }
    Modifier& setfixedHeight(float px)              { m_fixedHeight = px; return *this; }
    Modifier& align(Align alignment)                { m_alignment = alignment; return *this; }
    Modifier& setColor(sf::Color color)             { m_color = color; return *this; }
    Modifier& onClick(std::function<void()> cb)     { m_onClick = std::move(cb); return *this; }
    Modifier& setVisible(bool visible)              { m_isVisible = visible; return *this; }

    float getWidth() const                          { return m_widthPct; }
    float getHeight() const                         { return m_heightPct; }
    float getFixedWidth() const                     { return m_fixedWidth; }
    float getFixedHeight() const                    { return m_fixedHeight; }
    Align getAlignment() const                      { return m_alignment; }
    sf::Color getColor() const                      { return m_color; }
    const std::function<void()>& getOnClick() const { return m_onClick; }
    bool isVisible() const                          { return m_isVisible; }

private:
    float m_widthPct = 1.f;
    float m_heightPct = 1.f;
    float m_fixedWidth = 0.f;
    float m_fixedHeight = 0.f;
    Align m_alignment = Align::NONE;
    sf::Color m_color = sf::Color::Transparent;
    std::function<void()> m_onClick = nullptr;
    bool m_isVisible = true;
};

inline const Modifier default_mod;



// ---------------------------------------------------------------------------- //
// Element Type Enum
// ---------------------------------------------------------------------------- //
enum class EType {
    Element,
    Row,
    Column,
    Text,
    // ...
};



// ---------------------------------------------------------------------------- //
// Base Element Class
// ---------------------------------------------------------------------------- //
class Element {
public:
    sf::RectangleShape m_bounds;
    Modifier m_modifier;

    Element() {
        uilo_owned_elements.insert(this);
    }

    virtual ~Element() {
        if (uilo_owned_elements.find(this) != uilo_owned_elements.end() && !time_to_delete) {
            std::cerr << "[UILO] Error: Attempted to delete a UILO-owned element directly.\n";
            std::abort();
        }
    }

    virtual void update(sf::RectangleShape& parentBounds) {}
    virtual void render(sf::RenderTarget& target) {}

    virtual void handleEvent(const sf::Event& event) {
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

    virtual void checkClick(const sf::Vector2f& pos) {
        if (m_bounds.getGlobalBounds().contains(pos)) {
            if (m_modifier.getOnClick())
                m_modifier.getOnClick()();
        }
    }

    void setModifier(const Modifier& modifier) {
        m_modifier = modifier;
    }

    virtual EType getType() const {
        return EType::Element;
    }

protected:
    void alignResize(sf::RectangleShape& parent) {
        m_bounds.setSize(parent.getSize());
        m_bounds.setPosition(parent.getPosition());

        // Resize X
        if (m_modifier.getFixedWidth() != 0) {
            m_bounds.setSize({
                m_modifier.getFixedWidth(),
                m_bounds.getSize().y
            });
        } else {
            m_bounds.setSize({
                m_bounds.getSize().x * m_modifier.getWidth(),
                m_bounds.getSize().y * m_modifier.getHeight()
            });
        }

        // Resize Y
        if (m_modifier.getFixedHeight() != 0) {
            m_bounds.setSize({
                m_bounds.getSize().x,
                m_modifier.getFixedHeight()
            });
        } else {
            m_bounds.setSize({
                m_bounds.getSize().x * m_modifier.getWidth(),
                m_bounds.getSize().y * m_modifier.getHeight()
            });
        }
    }

    void applyModifiers() {
        m_bounds.setFillColor(m_modifier.getColor());
    }
};



// ---------------------------------------------------------------------------- //
// Container Base Class
// ---------------------------------------------------------------------------- //
class Container : public Element {
public:
    Container(std::initializer_list<Element*> elements = {}) {
        for (auto& e : elements)
            m_elements.push_back(e);

        uilo_owned_elements.insert(this);
    }

    Container(Modifier modifier = default_mod, std::initializer_list<Element*> elements = {}) {
        m_modifier = modifier;
        for (auto& e : elements)
            m_elements.push_back(e);

        uilo_owned_elements.insert(this);
    }

    ~Container() {
        if (uilo_owned_elements.find(this) != uilo_owned_elements.end() && !time_to_delete) {
            std::cerr << "[UILO] Error: Attempted to delete a UILO-owned container directly.\n";
            std::abort();
        }
    }

    void addElement(Element* element) {
        m_elements.push_back(element);
    }

    void addElements(std::initializer_list<Element*> elements) {
        for (auto& e : elements)
            m_elements.push_back(e);
    }

    virtual void handleEvent(const sf::Event& event) override {
        for (auto& e : m_elements)
            e->handleEvent(event);
        Element::handleEvent(event);
    }

    const std::vector<Element*>& getElements() const {
        return m_elements;
    }

protected:
    std::vector<Element*> m_elements;
};



// ---------------------------------------------------------------------------- //
// Row Container
// ---------------------------------------------------------------------------- //
class Row : public Container {
public:
    using Container::Container;

    ~Row() {
        if (uilo_owned_elements.find(this) != uilo_owned_elements.end() && !time_to_delete) {
            std::cerr << "[UILO] Error: Attempted to delete a UILO-owned row directly.\n";
            std::abort();
        }
    }

    void update(sf::RectangleShape& parentBounds) override {
        alignResize(parentBounds);
        applyModifiers();

        std::vector<Element*> leftElements;
        std::vector<Element*> centerElements;
        std::vector<Element*> rightElements;

        for (auto& e : m_elements) {
            auto align = e->m_modifier.getAlignment();
            if      (hasAlign(align, Align::LEFT))      leftElements.push_back(e);
            else if (hasAlign(align, Align::RIGHT))     rightElements.push_back(e);
            else if (hasAlign(align, Align::CENTER_X))  centerElements.push_back(e);
            else                                        leftElements.push_back(e); // default
        }

        float xLeft = m_bounds.getPosition().x;
        for (auto& e : leftElements) {
            sf::RectangleShape subBounds;
            subBounds.setPosition({ xLeft, m_bounds.getPosition().y });
            subBounds.setSize({
                m_bounds.getSize().x - (xLeft - m_bounds.getPosition().x),
                m_bounds.getSize().y
            });

            e->update(subBounds);
            e->m_bounds.setPosition({ xLeft, m_bounds.getPosition().y });
            xLeft += e->m_bounds.getSize().x;
        }

        float centerTotalWidth = 0.f;
        for (auto& e : centerElements) {
            e->update(m_bounds);
            centerTotalWidth += e->m_bounds.getSize().x;
        }

        float xCenter = m_bounds.getPosition().x + (m_bounds.getSize().x / 2.f) - (centerTotalWidth / 2.f);
        for (auto& e : centerElements) {
            e->m_bounds.setPosition({ xCenter, m_bounds.getPosition().y });
            xCenter += e->m_bounds.getSize().x;
        }

        float xRight = m_bounds.getPosition().x + m_bounds.getSize().x;
        for (auto& e : rightElements) {
            float maxWidth = xRight - m_bounds.getPosition().x;

            sf::RectangleShape subBounds;
            subBounds.setPosition({ m_bounds.getPosition().x, m_bounds.getPosition().y });
            subBounds.setSize({ maxWidth, m_bounds.getSize().y });

            e->update(subBounds);
            xRight -= e->m_bounds.getSize().x;
            e->m_bounds.setPosition({ xRight, m_bounds.getPosition().y });
        }
    }

    void render(sf::RenderTarget& target) override {
        target.draw(m_bounds);
        for (auto& e : m_elements) {
            if (e->m_modifier.isVisible())
                e->render(target);
        }
    }

    virtual void handleEvent(const sf::Event& event) override {
        for (auto& e : m_elements)
            e->handleEvent(event);
        Element::handleEvent(event);
    }

    void checkClick(const sf::Vector2f& pos) override {
        for (auto& e : m_elements)
            e->checkClick(pos);
        Element::checkClick(pos);
    }

    virtual EType getType() const override {
        return EType::Row;
    }
};



// ---------------------------------------------------------------------------- //
// Column Container
// ---------------------------------------------------------------------------- //
class Column : public Container {
public:
    using Container::Container;

    ~Column() {
        if (uilo_owned_elements.find(this) != uilo_owned_elements.end() && !time_to_delete) {
            std::cerr << "[UILO] Error: Attempted to delete a UILO-owned column directly.\n";
            std::abort();
        }
    }

    void update(sf::RectangleShape& parentBounds) override {
        alignResize(parentBounds);
        applyModifiers();

        std::vector<Element*> topElements;
        std::vector<Element*> centerElements;
        std::vector<Element*> bottomElements;

        for (auto& e : m_elements) {
            auto align = e->m_modifier.getAlignment();
            if      (hasAlign(align, Align::TOP))        topElements.push_back(e);
            else if (hasAlign(align, Align::BOTTOM))     bottomElements.push_back(e);
            else if (hasAlign(align, Align::CENTER_Y))   centerElements.push_back(e);
            else                                          topElements.push_back(e); // default
        }

        auto asyncUpdate = [](std::vector<Element*>& group, sf::RectangleShape& bounds) {
            std::vector<std::future<void>> futures;
            for (auto* e : group) {
                futures.emplace_back(std::async(std::launch::async, [e, &bounds]() {
                    e->update(bounds);
                }));
            }
            for (auto& f : futures) f.get();
        };

        auto futTop    = std::async(std::launch::async, asyncUpdate, std::ref(topElements), std::ref(m_bounds));
        auto futCenter = std::async(std::launch::async, asyncUpdate, std::ref(centerElements), std::ref(m_bounds));
        auto futBottom = std::async(std::launch::async, asyncUpdate, std::ref(bottomElements), std::ref(m_bounds));

        futTop.get();
        futCenter.get();
        futBottom.get();

        float yTop = m_bounds.getPosition().y;
        for (auto& e : topElements) {
            e->m_bounds.setPosition({ m_bounds.getPosition().x, yTop });
            yTop += e->m_bounds.getSize().y;
        }

        float centerTotalHeight = 0.f;
        for (auto& e : centerElements)
            centerTotalHeight += e->m_bounds.getSize().y;

        float yCenter = m_bounds.getPosition().y + (m_bounds.getSize().y / 2.f) - (centerTotalHeight / 2.f);
        for (auto& e : centerElements) {
            e->m_bounds.setPosition({ m_bounds.getPosition().x, yCenter });
            yCenter += e->m_bounds.getSize().y;
        }

        float yBottom = m_bounds.getPosition().y + m_bounds.getSize().y;
        for (auto& e : bottomElements) {
            yBottom -= e->m_bounds.getSize().y;
            e->m_bounds.setPosition({ m_bounds.getPosition().x, yBottom });
        }
    }


    void render(sf::RenderTarget& target) override {
        target.draw(m_bounds);
        for (auto& e : m_elements) {
            if (e->m_modifier.isVisible())
                e->render(target);
        }
    }

    virtual void handleEvent(const sf::Event& event) override {
        for (auto& e : m_elements)
            e->handleEvent(event);
        Element::handleEvent(event);
    }

    void checkClick(const sf::Vector2f& pos) override {
        for (auto& e : m_elements)
            e->checkClick(pos);
        Element::checkClick(pos);
    }

    virtual EType getType() const override {
        return EType::Column;
    }
};
    



// ---------------------------------------------------------------------------- //
// Text Element (WIP / Placeholder)
// ---------------------------------------------------------------------------- //
class Text : public Element {
public:

private:

};



// ---------------------------------------------------------------------------- //
// Spacer Element (WIP / Placeholder)
// ---------------------------------------------------------------------------- //
class Spacer : public Element {
public:
    Spacer(Modifier& modifier) { m_modifier = modifier; uilo_owned_elements.insert(this); }

    ~Spacer() override {
        if (uilo_owned_elements.find(this) != uilo_owned_elements.end() && !time_to_delete) {
            std::cerr << "[UILO] Error: Attempted to delete a UILO-owned spacer directly.\n";
            std::abort();
        }
    }
    
    void update(sf::RectangleShape& parentBounds) override {
        m_bounds.setFillColor(sf::Color::Transparent);

        alignResize(parentBounds);
        applyModifiers();
    }
};



// ---------------------------------------------------------------------------- //
// Page View
// ---------------------------------------------------------------------------- //
class Page {
public:
    Page() = default;

    Page(std::initializer_list<Container*> containers = {}) {
        m_bounds.setFillColor(sf::Color::Transparent);

        for (const auto& c : containers) 
            m_containers.push_back(c);

        uilo_owned_pages.insert(this);
    }

    ~Page() {
        if (uilo_owned_pages.find(this) != uilo_owned_pages.end() && !time_to_delete) {
            std::cerr << "[UILO] Error: Attempted to delete a UILO-owned page directly.\n";
            std::abort();
        }
    }

    void update(const sf::RectangleShape& parentBounds) {
        m_bounds = parentBounds;
    
        std::vector<std::future<void>> futures;
    
        for (auto& c : m_containers) {
            futures.emplace_back(std::async(std::launch::async, [c, this]() {
                c->update(m_bounds);
            }));
        }
    
        for (auto& f : futures)
            f.get();
    }

    void render(sf::RenderTarget& target) {
        target.draw(m_bounds);

        for (auto& c : m_containers) {
            if (c->m_modifier.isVisible())
                c->render(target);
        }
    }

    void handleEvent(const sf::Event& event) {
        for (auto& c : m_containers)
            c->handleEvent(event);
    }

    void dispatchClick(const sf::Vector2f& pos) {
        for (auto& c : m_containers)
            c->checkClick(pos);
    }

private:
    std::vector<Container*> m_containers;
    sf::RectangleShape m_bounds;
};



// ---------------------------------------------------------------------------- //
// UILO Application Core
// ---------------------------------------------------------------------------- //
class UILO {
public:
    UILO() {
        initDefaultView();

        m_window.create(
            m_defScreenRes, m_windowTitle,
            sf::Style::Resize | sf::Style::Close
        );

        m_window.setVerticalSyncEnabled(true);
        m_window.setView(m_defaultView);

        if (m_window.isOpen()) m_running = true;

        m_bounds.setFillColor(sf::Color::Transparent);
    }

    UILO(const std::string& windowTitle = "", std::initializer_list<std::pair<Page*, std::string>> pages = {})
    : m_windowTitle(windowTitle) {
        initDefaultView();

        m_window.create(
            m_defScreenRes, m_windowTitle,
            sf::Style::Resize | sf::Style::Close
        );

        m_window.setVerticalSyncEnabled(true);
        m_window.setView(m_defaultView);

        if (m_window.isOpen()) m_running = true;

        for (auto& [page, name] : pages) {
            if (uilo_owned_pages.find(page) != uilo_owned_pages.end()) {
                uilo_owned_pages.insert(page);
            }
            m_pages[name] = page;
            if (!m_currentPage) m_currentPage = page;
        }

        m_bounds.setFillColor(sf::Color::Transparent);
    }

    UILO(sf::RenderWindow& userWindow, sf::View& windowView, std::initializer_list<std::pair<Page*, std::string>> pages = {}) {
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
            m_pages[name] = page;
            if (!m_currentPage) m_currentPage = page;
        }

        m_bounds.setFillColor(sf::Color::Transparent);
    }

    ~UILO() {
        time_to_delete = true;

        for (auto& [name, page] : m_pages) {
            delete page;
            uilo_owned_pages.erase(page);
        }
    }

    void update() {
        pollEvents();

        if (!m_windowOwned)
            return;

        sf::Vector2u currentSize = m_window.getSize();
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
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

        if (m_clickPosition) {
            m_currentPage->dispatchClick(*m_clickPosition);
            m_clickPosition.reset();
        }
    }

    void update(sf::View& windowView) {
        pollEvents();

        if (m_windowOwned)
            return;

        sf::Vector2u currentSize = m_userWindow->getSize();
        if (m_shouldUpdate) {
            windowView.setSize({ (float)currentSize.x, (float)currentSize.y });

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

    void render() {
        if (m_windowOwned) {
            if (m_shouldUpdate) {
                m_window.clear(sf::Color::Black);
                m_currentPage->render(m_window);
                m_window.display();
            }
        }
        else
            m_currentPage->render(*m_userWindow);

        m_shouldUpdate = false;
    }

    void setTitle(const std::string& newTitle) {
        m_window.setTitle(newTitle);
    }

    bool isRunning() const {
        return m_running;
    }

    void addPage(std::pair<Page*&, std::string> newPage) {
        Page*& page = newPage.first;
        const std::string& name = newPage.second;

        if (uilo_owned_pages.find(page) == uilo_owned_pages.end()) {
            uilo_owned_pages.insert(page);
        }
        m_pages[name] = page;
        if (!m_currentPage) m_currentPage = page;
    }

    void addPages(std::initializer_list<std::pair<Page*&, std::string>> pages) {
        for (const auto& [page, name] : pages) {
            if (uilo_owned_pages.find(page) == uilo_owned_pages.end()) {
                uilo_owned_pages.insert(page);
            }
            m_pages[name] = page;
            if (!m_currentPage) m_currentPage = page;
        }
    }

    void switchToPage(const std::string& pageName) {
        auto it = m_pages.find(pageName);
        if (it != m_pages.end()) {
            m_currentPage = it->second;
            m_currentPage->update(m_bounds);
        } else {
            std::cerr << "[UILO] Page \"" << pageName << "\" not found.\n";
        }
    }

private:
    sf::RenderWindow m_window;
    sf::RenderWindow* m_userWindow = nullptr;
    bool m_windowOwned = true;
    sf::VideoMode m_defScreenRes;
    sf::View m_defaultView;
    sf::RectangleShape m_bounds;
    std::string m_windowTitle = "";

    std::unordered_map<std::string, Page*> m_pages;
    Page* m_currentPage = nullptr;

    bool m_running       = false;
    bool m_shouldUpdate  = false;

    sf::Vector2u m_lastWindowSize;
    std::optional<sf::Vector2f> m_clickPosition;

    void pollEvents() {
        while (const auto event = (m_windowOwned) ?  m_window.pollEvent() : m_userWindow->pollEvent()) {
            if (!m_windowOwned && !m_userWindow)
                return;

            if (event->is<sf::Event::Closed>()) {
                if (m_windowOwned) m_window.close();
                else m_userWindow->close();
                m_running = false;
                shutdown();
            }

            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    if (m_windowOwned) m_clickPosition = m_window.mapPixelToCoords(mousePressed->position);
                    else m_clickPosition = m_userWindow->mapPixelToCoords(mousePressed->position);
                }

                m_shouldUpdate = true;
            }

            if (event->is<sf::Event::Resized>())
                m_shouldUpdate = true;
        }
    }

    void initDefaultView() {
        m_defScreenRes = sf::VideoMode::getDesktopMode();
        m_defScreenRes.size.x /= 2;
        m_defScreenRes.size.y /= 2;
    
        m_defaultView.setSize({
            (float)m_defScreenRes.size.x,
            (float)m_defScreenRes.size.y
        });
    }

    void setView(const sf::View& view) {
        m_defaultView = view;
        if (!m_windowOwned)
            m_userWindow->setView(m_defaultView);
    }

    void shutdown() {
        time_to_delete = true;
        for (auto& e : uilo_owned_elements) {
            delete e;
        }
        for (auto& [name, page] : m_pages) {
            if (uilo_owned_pages.count(page)) {
                delete page;
                uilo_owned_pages.erase(page);
            }
        }
    
        m_pages.clear();
        uilo_owned_elements.clear();
        time_to_delete = false;
    }    
};

} // !namespace uilo

#endif // !UILO_HPP