#pragma once

#include <functional>
#include "Shape.hpp"

namespace uilo {

class Text;
class Image;

class Renderer {
public:
    Renderer() = default;

    void setDrawFilledRect(std::function<void(Rect*)> fn)                     { m_drawFilledRect = fn; }
    void setDrawOutlineRect(std::function<void(Rect*, float)> fn)             { m_drawOutlineRect = fn; }
    void setDrawRoundedRect(std::function<void(Rect*, float)> fn)             { m_drawRoundedRect = fn; }
    void setDrawFilledCircle(std::function<void(Circle*)> fn)                 { m_drawFilledCircle = fn; }
    void setDrawOutlineCircle(std::function<void(Circle*, float)> fn)         { m_drawOutlineCircle = fn; }
    void setDrawText(std::function<void(Text*)> fn)                           { m_drawText = fn; }
    void setDrawImage(std::function<void(Image*)> fn)                          { m_drawImage = fn; }
    void setPushClipRect(std::function<void(const Bounds&)> fn)               { m_pushClipRect = fn; }
    void setPushClipRoundedRect(std::function<void(const Bounds&, float)> fn) { m_pushClipRoundedRect = fn; }
    void setPopClip(std::function<void()> fn)                                 { m_popClip = fn; }
    void setBeginFrame(std::function<void()> fn)                              { m_beginFrame = fn; }

    void drawFilledRect(Rect& rect) {
        if (!m_drawFilledRect) return;
        if (m_renderScale == 1.f) {
            m_drawFilledRect(&rect);
            return;
        }
        Rect scaled;
        applyScale(rect, scaled);
        m_drawFilledRect(&scaled);
    }

    void drawOutlineRect(Rect& rect, float thickness) {
        if (!m_drawOutlineRect) return;
        if (m_renderScale == 1.f) {
            m_drawOutlineRect(&rect, thickness);
            return;
        }
        Rect scaled;
        applyScale(rect, scaled);
        m_drawOutlineRect(&scaled, thickness * m_renderScale);
    }

    void drawRoundedRect(Rect& rect, float radius) {
        if (!m_drawRoundedRect) return;
        if (m_renderScale == 1.f) {
            m_drawRoundedRect(&rect, radius);
            return;
        }
        Rect scaled;
        applyScale(rect, scaled);
        m_drawRoundedRect(&scaled, radius * m_renderScale);
    }

    void drawFilledCircle(Circle& circle) {
        if (!m_drawFilledCircle) return;
        if (m_renderScale == 1.f) {
            m_drawFilledCircle(&circle);
            return;
        }
        Circle scaled;
        applyScale(circle, scaled);
        m_drawFilledCircle(&scaled);
    }

    void drawOutlineCircle(Circle& circle, float thickness) {
        if (!m_drawOutlineCircle) return;
        if (m_renderScale == 1.f) {
            m_drawOutlineCircle(&circle, thickness);
            return;
        }
        Circle scaled;
        applyScale(circle, scaled);
        m_drawOutlineCircle(&scaled, thickness * m_renderScale);
    }

    void drawText(Text& text) {
        if (m_drawText) m_drawText(&text);
    }

    void drawImage(Image& image) {
        if (m_drawImage) m_drawImage(&image);
    }

    void pushClipRect(const Bounds& bounds) {
        if (!m_pushClipRect) return;
        if (m_renderScale == 1.f) {
            m_pushClipRect(bounds);
            return;
        }
        Bounds s;
        s.position = bounds.position.mul(m_renderScale);
        s.size = bounds.size.mul(m_renderScale);
        m_pushClipRect(s);
    }

    void pushClipRoundedRect(const Bounds& bounds, float radius) {
        if (!m_pushClipRoundedRect) return;
        if (m_renderScale == 1.f) {
            m_pushClipRoundedRect(bounds, radius);
            return;
        }
        Bounds s;
        s.position = bounds.position.mul(m_renderScale);
        s.size = bounds.size.mul(m_renderScale);
        m_pushClipRoundedRect(s, radius * m_renderScale);
    }

    void popClip() {
        if (m_popClip) m_popClip();
    }

    void beginFrame() {
        if (m_beginFrame) m_beginFrame();
    }

protected:
    std::function<void(Rect*)>               m_drawFilledRect = nullptr;
    std::function<void(Rect*, float)>        m_drawOutlineRect = nullptr;
    std::function<void(Rect*, float)>        m_drawRoundedRect = nullptr;
    std::function<void(Circle*)>             m_drawFilledCircle = nullptr;
    std::function<void(Circle*, float)>      m_drawOutlineCircle = nullptr;
    std::function<void(Text*)>               m_drawText = nullptr;
    std::function<void(Image*)>              m_drawImage = nullptr;
    std::function<void(const Bounds&)>       m_pushClipRect = nullptr;
    std::function<void(const Bounds&, float)> m_pushClipRoundedRect = nullptr;
    std::function<void()>                    m_popClip = nullptr;
    std::function<void()>                    m_beginFrame = nullptr;

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

} // namespace uilo