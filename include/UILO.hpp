#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Page.hpp"
#include "input/Input.hpp"

namespace uilo {

class UILO {
public:
    UILO();

    void update(const Input& input);
    void render(Renderer& renderer);

    void addPage(Page* page);
    void setPage(const std::string& pageName);
    void setScreenBounds(const Bounds& bounds);
    void setOnResize(std::function<void(float, float)> callback);

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
    std::chrono::steady_clock::time_point m_lastTime;
    std::function<void(float, float)> m_onResize;

    friend class Element;
};

}