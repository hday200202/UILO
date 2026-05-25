#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

#include "Elements.hpp"

namespace uilo {

class Interactible;

class UILO {
public:
    UILO() = default;
    UILO(sf::RenderWindow& window, Page* page);

    void update();
    void render();
    void handleEvent(const sf::Event& event);

    void setRenderWindow(sf::RenderWindow& window) { m_window = &window; }

    void addPage(Page* page);
    void setPage(const std::string& pageName);
    void setScale(float scale);

    void registerOverlay(Element* e, std::function<void()> onDismiss = {});
    void unregisterOverlay(Element* e);

    void setCurrInteractible(Interactible* i);
    Interactible* getCurrInteractible() const { return m_currInteractible; }
    
    float getScale() const { return m_scale; }
    float getDeltaTime() const { return m_deltaTime; }
    sf::Vector2u getWindowSize() const { return m_window ? m_window->getSize() : sf::Vector2u{0u, 0u}; }
    sf::Vector2f getMousePosition() const {
        sf::Vector2i raw = sf::Mouse::getPosition(*m_window);
        return { static_cast<float>(raw.x), static_cast<float>(raw.y) };
    }

    void requestCursor(sf::Cursor::Type type, int priority = 0);

    template <typename T>
    T* getElement(const std::string& name) {
        auto it = m_elements.find(name);
        if (it == m_elements.end()) return nullptr;
        return dynamic_cast<T*>(it->second);
    }

private:
    std::vector<std::unique_ptr<Element>>                   m_elementPool;
    std::unordered_map<std::string, Element*>               m_elements;
    std::unordered_map<std::string, std::unique_ptr<Page>>  m_pages;

    struct OverlayEntry {
        Element*              element;
        std::function<void()> onDismiss;
    };
    std::vector<OverlayEntry> m_overlays;
    std::vector<Element*>     m_resizers;

    Page* m_activePage = nullptr;

    float m_scale = 1.f;
    float m_deltaTime = 0.f;

    Timer m_timer;

    sf::RenderWindow* m_window = nullptr;

    bool m_prevLeftMouse  = false;
    bool m_prevRightMouse = false;

    sf::Cursor::Type          m_pendingCursor         = sf::Cursor::Type::Arrow;
    int                       m_pendingCursorPriority = 0;
    sf::Cursor::Type          m_activeCursor          = sf::Cursor::Type::Arrow;
    std::optional<sf::Cursor> m_curArrow;
    std::optional<sf::Cursor> m_curHand;
    std::optional<sf::Cursor> m_curSizeH;
    std::optional<sf::Cursor> m_curSizeV;

    Interactible* m_currInteractible             = nullptr;
    bool          m_interactibleActivatedThisFrame = false;

    friend class Element;
    friend class Interactible;
};

}