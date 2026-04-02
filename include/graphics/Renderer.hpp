#pragma once

#include <functional>
#include "Shape.hpp"

namespace uilo {

class Text;

class Renderer {
public:
    Renderer() = default;

    void setDrawFilledRect(std::function<void(Rect*)> fn)               { m_drawFilledRect      = fn; }
    void setDrawOutlineRect(std::function<void(Rect*, float)> fn)       { m_drawOutlineRect     = fn; }
    void setDrawRoundedRect(std::function<void(Rect*, float)> fn)       { m_drawRoundedRect     = fn; }
    void setDrawFilledCircle(std::function<void(Circle*)> fn)           { m_drawFilledCircle    = fn; }
    void setDrawOutlineCircle(std::function<void(Circle*, float)> fn)   { m_drawOutlineCircle   = fn; }
    void setDrawText(std::function<void(Text*)> fn)                     { m_drawText            = fn; }

    void drawFilledRect(Rect& rect) {
        if (!m_drawFilledRect) return;
        if (m_renderScale == 1.f) { m_drawFilledRect(&rect); return; }
        Rect scaled; applyScale(rect, scaled);
        m_drawFilledRect(&scaled);
    }
    void drawOutlineRect(Rect& rect, float thickness) {
        if (!m_drawOutlineRect) return;
        if (m_renderScale == 1.f) { m_drawOutlineRect(&rect, thickness); return; }
        Rect scaled; applyScale(rect, scaled);
        m_drawOutlineRect(&scaled, thickness * m_renderScale);
    }
    void drawRoundedRect(Rect& rect, float radius) {
        if (!m_drawRoundedRect) return;
        if (m_renderScale == 1.f) { m_drawRoundedRect(&rect, radius); return; }
        Rect scaled; applyScale(rect, scaled);
        m_drawRoundedRect(&scaled, radius * m_renderScale);
    }
    void drawFilledCircle(Circle& circle) {
        if (!m_drawFilledCircle) return;
        if (m_renderScale == 1.f) { m_drawFilledCircle(&circle); return; }
        Circle scaled; applyScale(circle, scaled);
        m_drawFilledCircle(&scaled);
    }
    void drawOutlineCircle(Circle& circle, float thickness) {
        if (!m_drawOutlineCircle) return;
        if (m_renderScale == 1.f) { m_drawOutlineCircle(&circle, thickness); return; }
        Circle scaled; applyScale(circle, scaled);
        m_drawOutlineCircle(&scaled, thickness * m_renderScale);
    }
    void drawText(Text& text)                                           { if (m_drawText) m_drawText(&text); }

protected:
    std::function<void(Rect*)>              m_drawFilledRect            = nullptr;
    std::function<void(Rect*, float)>       m_drawOutlineRect           = nullptr;
    std::function<void(Rect*, float)>       m_drawRoundedRect           = nullptr;
    std::function<void(Circle*)>            m_drawFilledCircle          = nullptr;
    std::function<void(Circle*, float)>     m_drawOutlineCircle         = nullptr;
    std::function<void(Text*)>              m_drawText                  = nullptr;

    float m_renderScale = 1.f;

    void applyScale(const Rect& src, Rect& dst) {
        dst.setColor(src.getColor());
        dst.setPosition({src.getPosition().x * m_renderScale, src.getPosition().y * m_renderScale});
        dst.setSize({src.getSize().x * m_renderScale, src.getSize().y * m_renderScale});
    }
    void applyScale(const Circle& src, Circle& dst) {
        dst.setColor(src.getColor());
        dst.setPosition({src.getPosition().x * m_renderScale, src.getPosition().y * m_renderScale});
        dst.setSize({src.getSize().x * m_renderScale, src.getSize().y * m_renderScale});
    }

    friend class UILO;
};

}