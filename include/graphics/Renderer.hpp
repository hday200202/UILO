#pragma once

#include <functional>
#include "Shape.hpp"

namespace uilo {

class Renderer {
public:
    Renderer() = default;

    void setDrawFilledRect(std::function<void(Rect*)> fn)    { m_drawFilledRect = fn; }
    void setDrawOutlineRect(std::function<void(Rect*, float)> fn) { m_drawOutlineRect = fn; }
    void setDrawRoundedRect(std::function<void(Rect*, float)> fn) { m_drawRoundedRect = fn; }
    void setDrawFilledCircle(std::function<void(Circle*)> fn) { m_drawFilledCircle = fn; }
    void setDrawOutlineCircle(std::function<void(Circle*, float)> fn) { m_drawOutlineCircle = fn; }

    void drawFilledRect(Rect& rect)                         { if (m_drawFilledRect)    m_drawFilledRect(&rect); }
    void drawOutlineRect(Rect& rect, float thickness)       { if (m_drawOutlineRect)   m_drawOutlineRect(&rect, thickness); }
    void drawRoundedRect(Rect& rect, float radius)          { if (m_drawRoundedRect)   m_drawRoundedRect(&rect, radius); }
    void drawFilledCircle(Circle& circle)                   { if (m_drawFilledCircle)  m_drawFilledCircle(&circle); }
    void drawOutlineCircle(Circle& circle, float thickness) { if (m_drawOutlineCircle) m_drawOutlineCircle(&circle, thickness); }

protected:
    std::function<void(Rect*)>          m_drawFilledRect    = nullptr;
    std::function<void(Rect*, float)>   m_drawOutlineRect   = nullptr;
    std::function<void(Rect*, float)>   m_drawRoundedRect   = nullptr;
    std::function<void(Circle*)>        m_drawFilledCircle  = nullptr;
    std::function<void(Circle*, float)> m_drawOutlineCircle = nullptr;
};

}