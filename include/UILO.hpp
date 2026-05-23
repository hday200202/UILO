#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include "Elements.hpp"

namespace uilo {

class UILO {
public:
    UILO() = default;
    UILO(sf::RenderWindow& window, Page* page);

    void update();
    void render();

    void setRenderWindow(sf::RenderWindow& window) { m_window = &window; }

    void addPage(Page* page);
    void setPage(const std::string& pageName);
    void setScale(float scale);
    float getScale() const { return m_scale; }
    float getDeltaTime() const { return m_deltaTime; }

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

    Page* m_activePage = nullptr;

    float m_scale = 1.f;
    float m_deltaTime = 0.f;

    Timer m_timer;

    sf::RenderWindow* m_window = nullptr;

    bool m_prevLeftMouse  = false;
    bool m_prevRightMouse = false;

    friend class Element;
};

}