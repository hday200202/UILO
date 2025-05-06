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
#include <memory>
#include <algorithm>

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
    sf::RectangleShape m_bounds;
    Modifier m_modifier;

    virtual ~Element() = default;
    virtual void update(sf::RectangleShape& parentBounds) {}
    virtual void render(sf::RenderTarget& target) {}

protected:
    void alignResize(sf::RectangleShape& parent)
    {
        m_bounds.setSize(parent.getSize());
        m_bounds.setPosition(parent.getPosition());

        // Resize X
        if (m_modifier.getFixedWidth() != 0)
            m_bounds.setSize
            ({
                m_modifier.getFixedWidth(), 
                m_bounds.getSize().y
            });
        else
            m_bounds.setSize
            ({
                m_bounds.getSize().x * m_modifier.getWidth(), 
                m_bounds.getSize().y * m_modifier.getHeight()
            });

        // Resize Y
        if (m_modifier.getFixedHeight() != 0)
            m_bounds.setSize
            ({
                m_bounds.getSize().x,
                m_modifier.getFixedHeight()
            });
        else
            m_bounds.setSize
            ({
                m_bounds.getSize().x * m_modifier.getWidth(),
                m_bounds.getSize().y * m_modifier.getHeight()
            });

        // Align X
        if (hasAlign(m_modifier.getAlignment(), Align::CENTER_X))
            m_bounds.setPosition
            ({
                parent.getPosition().x + (parent.getSize().x / 2) - (m_bounds.getSize().x / 2),
                m_bounds.getPosition().y
            });
        else if (hasAlign(m_modifier.getAlignment(), Align::LEFT))
            m_bounds.setPosition
            ({
                parent.getPosition().x, 
                parent.getPosition().y
            });
        else if (hasAlign(m_modifier.getAlignment(), Align::RIGHT))
            m_bounds.setPosition
            ({
                parent.getPosition().x + parent.getSize().x - m_bounds.getSize().x,
                parent.getPosition().y
            });
        
        // Align Y
        if (hasAlign(m_modifier.getAlignment(), Align::CENTER_Y))
            m_bounds.setPosition
            ({
                m_bounds.getPosition().x,
                parent.getPosition().y + (parent.getSize().y / 2) - (m_bounds.getSize().y / 2)
            });
        else if (hasAlign(m_modifier.getAlignment(), Align::TOP))
            m_bounds.setPosition
            ({
                m_bounds.getPosition().x,
                parent.getPosition().y
            });
        else if (hasAlign(m_modifier.getAlignment(), Align::BOTTOM))
            m_bounds.setPosition
            ({
                m_bounds.getPosition().x,
                parent.getPosition().y + parent.getSize().y - m_bounds.getSize().y
            });
        
    }

    void applyModifiers()
    {
        m_bounds.setFillColor(m_modifier.getColor());
    }
};
// ---------------------------------------------------------------------------- //



// ---------------------------------------------------------------------------- Containers
class Container : public Element 
{
public:
    Container(std::initializer_list<Element*> elements = {})
    {        
        for (auto& e : elements)
            m_elements.push_back(e);
    }

    Container(Modifier modifier = default_mod, std::initializer_list<Element*> elements = {})
    {
        m_modifier = modifier;

        for (auto& e : elements)
            m_elements.push_back(e);
    }

    ~Container()
    {
        for (auto& e : m_elements)
            delete e;
    }

protected:
    std::vector<Element*> m_elements;
    
};



class Row : public Container 
{
public:
    using Container::Container;

    void update(sf::RectangleShape& parentBounds) override
    {
        // Align and resize self, apply visual modifiers
        alignResize(parentBounds);
        applyModifiers();

        std::vector<Element*> leftElements;
        std::vector<Element*> centerElements;
        std::vector<Element*> rightElements;

        // Categorize elements
        for (auto& e : m_elements) {
            auto align = e->m_modifier.getAlignment();

            if (hasAlign(align, Align::LEFT)) leftElements.push_back(e);
            else if (hasAlign(align, Align::RIGHT)) rightElements.push_back(e);
            else if (hasAlign(align, Align::CENTER_X)) centerElements.push_back(e);
            else leftElements.push_back(e);
        }

        std::reverse(rightElements.begin(), rightElements.end());

        // Place top elements
        float xLeft = m_bounds.getPosition().x;
        for (auto& e : leftElements) {
            e->update(m_bounds);
            e->m_bounds.setPosition({ xLeft, m_bounds.getPosition().y });
            xLeft += e->m_bounds.getSize().x;
        }

        // Update elements
        for (auto& e : m_elements)
            e->update(m_bounds);
    }

    void render(sf::RenderTarget& target) override
    {
        // Render self
        target.draw(m_bounds);
        
        // Render elements
        for (auto& e : m_elements)
            e->render(target);
    }
    
};



class Column : public Container 
{
public:
    using Container::Container;

    void update(sf::RectangleShape& parentBounds) override
    {
        alignResize(parentBounds);
        applyModifiers();

        std::vector<Element*> topElements;
        std::vector<Element*> centerElements;
        std::vector<Element*> bottomElements;

        // Categorize elements
        for (auto& e : m_elements) {
            auto align = e->m_modifier.getAlignment();

            if (hasAlign(align, Align::TOP)) topElements.push_back(e);
            else if (hasAlign(align, Align::BOTTOM)) bottomElements.push_back(e);
            else if (hasAlign(align, Align::CENTER_Y)) centerElements.push_back(e);
            else topElements.push_back(e);
        }

        std::reverse(bottomElements.begin(), bottomElements.end());

        // Place top elements
        float yTop = m_bounds.getPosition().y;
        for (auto& e : topElements) {
            e->update(m_bounds);
            e->m_bounds.setPosition({ m_bounds.getPosition().x, yTop });
            yTop += e->m_bounds.getSize().y;
        }

        // Place center elements
        float centerTotalHeight = 0.f;
        for (auto& e : centerElements) {
            e->update(m_bounds);
            centerTotalHeight += e->m_bounds.getSize().y;
        }

        float yCenter = m_bounds.getPosition().y + (m_bounds.getSize().y / 2.f) - (centerTotalHeight / 2.f);
        for (auto& e : centerElements) {
            e->m_bounds.setPosition({ m_bounds.getPosition().x, yCenter });
            yCenter += e->m_bounds.getSize().y;
        }

        // Place bottom elements (in reverse order to stack upward)
        float yBottom = m_bounds.getPosition().y + m_bounds.getSize().y;
        for (auto it = bottomElements.rbegin(); it != bottomElements.rend(); ++it) {
            Element* e = *it;
            e->update(m_bounds);
            yBottom -= e->m_bounds.getSize().y;
            e->m_bounds.setPosition({ m_bounds.getPosition().x, yBottom });
        }
    }

    void render(sf::RenderTarget& target) override
    {
        // Render self
        target.draw(m_bounds);

        // Render elements
        for (auto& e : m_elements)
            e->render(target);
    }
    
};
// ---------------------------------------------------------------------------- //



// ---------------------------------------------------------------------------- Text / Spacers
class Text : public Element {
public:

private:
    
};
// ---------------------------------------------------------------------------- //



// ---------------------------------------------------------------------------- View
class Page 
{
public:
    Page() = default;
    Page(std::initializer_list<Container*> containers = {})
    {
        m_bounds.setFillColor(sf::Color::Transparent);
        for (const auto& c : containers)
            m_containers.push_back(c);
    }

    void update(const sf::RectangleShape& parentBounds)
    {
        // Page bounds are the same as window bounds
        m_bounds = parentBounds;

        // Update Containers
        for (auto& c : m_containers)
            c->update(m_bounds);
    }

    void render(sf::RenderTarget& target) 
    {
        // Draw self
        target.draw(m_bounds);

        // Draw elements
        for (auto& c : m_containers)
            c->render(target);
    }

private:
    std::vector<Container*> m_containers;
    sf::RectangleShape m_bounds;
};
// ---------------------------------------------------------------------------- //



// ---------------------------------------------------------------------------- UILO
class UILO 
{
public:
    UILO() 
    {
        m_defScreenRes = sf::VideoMode::getDesktopMode();
        m_defScreenRes.size.x /= 2;
        m_defScreenRes.size.y /= 2;

        m_defaultView.setSize
        ({
            (float)m_defScreenRes.size.x, 
            (float)m_defScreenRes.size.y
        });

        m_window.create
        (
            m_defScreenRes, m_windowTitle, 
            sf::Style::Resize | sf::Style::Close
        );

        m_window.setVerticalSyncEnabled(true);
        m_window.setView(m_defaultView);

        if (m_window.isOpen()) m_running = true;
        m_bounds.setFillColor(sf::Color::Transparent);
    }

    UILO(const std::string& windowTitle = "", std::initializer_list<std::pair<Page*, std::string>> pages = {})
    : m_windowTitle(windowTitle)
    {
        m_defScreenRes = sf::VideoMode::getDesktopMode();
        m_defScreenRes.size.x /= 2;
        m_defScreenRes.size.y /= 2;

        m_defaultView.setSize
        ({
            (float)m_defScreenRes.size.x, 
            (float)m_defScreenRes.size.y
        });

        m_window.create
        (
            m_defScreenRes, m_windowTitle, 
            sf::Style::Resize | sf::Style::Close
        );

        m_window.setVerticalSyncEnabled(true);
        m_window.setView(m_defaultView);

        if (m_window.isOpen()) m_running = true;

        for (const auto& [page, name] : pages)
        {
            m_pages[name] = page;
            if (!m_currentPage) m_currentPage = page;
        }

        m_bounds.setFillColor(sf::Color::Transparent);
    }

    ~UILO() 
    {
        for (auto& [name, page] : m_pages)
            delete page;
    }

    void update() 
    {
        pollEvents();

        m_defaultView.setSize
        ({
            (float)m_window.getSize().x, 
            (float)m_window.getSize().y
        });

        m_bounds.setSize(m_defaultView.getSize());
        m_bounds.setPosition
        ({
            m_defaultView.getCenter().x - (m_defaultView.getSize().x / 2), 
            m_defaultView.getCenter().y - (m_defaultView.getSize().y / 2)
        });
        
        m_currentPage->update(m_bounds);
        m_window.setView(m_defaultView);
    }

    void render() 
    {
        m_window.clear(sf::Color::Black);
        m_currentPage->render(m_window);
        m_window.display();
    }

    void setTitle(const std::string& newTitle) { m_window.setTitle(newTitle); }
    bool isRunning() const { return m_running; }

    void addPage(const std::pair<Page*, std::string>& newPage) {}

private:
    sf::RenderWindow m_window;
    sf::VideoMode m_defScreenRes;
    sf::View m_defaultView;
    sf::RectangleShape m_bounds;

    std::string m_windowTitle = "";
    std::unordered_map<std::string, Page*> m_pages;
    Page* m_currentPage = nullptr;

    bool m_running = false;
    // bool m_shouldUpdate = true;
    const unsigned int m_minWindowWidth = 800;
    const unsigned int m_minWindowHeight = 600;

    void pollEvents() 
    {
        while (const auto event = m_window.pollEvent()) 
        {
            if (event->is<sf::Event::Closed>()) 
            {
                m_window.close();
                m_running = false;
            }
        }
    }
};
// ---------------------------------------------------------------------------- //

} // !namespace uilo

#endif // !UILO_HPP