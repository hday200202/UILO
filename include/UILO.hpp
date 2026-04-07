#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Page.hpp"
#include "input/Input.hpp"
#include "input/Mouse.hpp"
#include "Factory.hpp"
#include "utils/Timer.hpp"

namespace uilo {

class UILO {
public:
    UILO();

    void update(const Input& input);
    bool update(Renderer& renderer);
    void render(Renderer& renderer);

    void addPage(Page* page);
    void setPage(const std::string& pageName);
    void setScreenBounds(const Bounds& bounds);
    void setOnResize(std::function<void(float, float)> callback);
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
    std::vector<std::unique_ptr<Element>>                   m_elementPool; // owns all elements
    std::unordered_map<std::string, Element*>               m_elements;    // named lookup
    std::unordered_map<std::string, std::unique_ptr<Page>>  m_pages;

    Page*  m_activePage = nullptr;
    Bounds m_screenBounds;
    float  m_scale = 1.f;
    Timer m_timer;
    std::function<void(float, float)> m_onResize;

    float m_deltaTime = 0.f;

    friend class Element;
};

} // namespace uilo

#ifdef UILO_SFML
#include "renderer_interfaces/UILO_SFML.hpp"
#endif

#ifdef UILO_SDL
#include "renderer_interfaces/UILO_SDL.hpp"
#endif