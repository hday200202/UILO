#pragma once

#include <SFML/Graphics.hpp>
#define GL_SILENCE_DEPRECATION
#include <SFML/OpenGL.hpp>
#include <cmath>
#include <vector>

#include "../graphics/Renderer.hpp"
#include "../elements/decor/Text.hpp"
#include "../elements/decor/Image.hpp"
#include "../utils/Alignment.hpp"
#include "../../assets/EmbeddedFont.hpp"

namespace uilo {

class SFMLRenderer : public Renderer {
public:
    // Returns context settings with the stencil buffer and MSAA required by this renderer.
    static sf::ContextSettings recommendedSettings() {
        sf::ContextSettings s;
        s.antiAliasingLevel = 8;
        s.stencilBits       = 8;
        return s;
    }

    explicit SFMLRenderer(sf::RenderWindow& window) : m_window(window) {
        if (!m_font.openFromMemory(
                uilo::EMBEDDED_DEJAVUSANS_FONT.data(),
                uilo::EMBEDDED_DEJAVUSANS_FONT.size()))
            return;
        setupCallbacks();
    }

private:
    sf::RenderWindow& m_window;
    sf::Font m_font;

    struct ClipEntry { sf::IntRect scissor; bool isRounded; Bounds bounds; float radius; };
    std::vector<ClipEntry> m_clipStack;
    int m_stencilDepth = 0;

    // Build a VertexArray for a rounded-rect shape (used for stencil writes).
    sf::VertexArray vaFromBounds(const Bounds& b, float r) {
        Rect shape;
        shape.setPosition({b.position.x, b.position.y});
        shape.setSize({b.size.x, b.size.y});
        shape.setCornerRadius(r);
        shape.setColor({255, 255, 255, 255});
        const auto& verts = shape.getVertices();
        const auto& idxs  = shape.getIndices();
        sf::VertexArray va(sf::PrimitiveType::Triangles, idxs.size());
        for (size_t i = 0; i < idxs.size(); i++) {
            va[i].position = {verts[idxs[i]].position.x, verts[idxs[i]].position.y};
            va[i].color    = sf::Color::White;
        }
        return va;
    }

    void updateGLState() {
        int winH = (int)m_window.getSize().y;
        if (m_clipStack.empty()) {
            glDisable(GL_SCISSOR_TEST);
        } else {
            const auto& r = m_clipStack.back().scissor;
            glEnable(GL_SCISSOR_TEST);
            glScissor(r.position.x, winH - r.position.y - r.size.y, r.size.x, r.size.y);
        }
        if (m_stencilDepth == 0) {
            glDisable(GL_STENCIL_TEST);
        } else {
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_EQUAL, m_stencilDepth, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        }
    }

    sf::IntRect computeScissor(const Bounds& b) {
        int rx1 = (int)std::floor(b.position.x);
        int ry1 = (int)std::floor(b.position.y);
        int rx2 = (int)std::ceil(b.position.x + b.size.x);
        int ry2 = (int)std::ceil(b.position.y + b.size.y);
        sf::IntRect rect({rx1, ry1}, {rx2 - rx1, ry2 - ry1});
        if (!m_clipStack.empty()) {
            const auto& cur = m_clipStack.back().scissor;
            int x1 = std::max(rect.position.x, cur.position.x);
            int y1 = std::max(rect.position.y, cur.position.y);
            int x2 = std::min(rect.position.x + rect.size.x, cur.position.x + cur.size.x);
            int y2 = std::min(rect.position.y + rect.size.y, cur.position.y + cur.size.y);
            rect = sf::IntRect({x1, y1}, {std::max(0, x2 - x1), std::max(0, y2 - y1)});
        }
        return rect;
    }

    void setupCallbacks() {
        // --- Draw ---
        setDrawFilledRect([this](Rect* r) { drawRectVA(r); });
        setDrawRoundedRect([this](Rect* r, float radius) {
            r->setCornerRadius(radius);
            drawRectVA(r);
        });
        setDrawText([this](Text* t) {
            const float s = m_renderScale;
            sf::Text sfText(m_font, t->getString(), (uint32_t)(t->getFontSize() * s));
            auto c = t->getModifier().getColor();
            sfText.setFillColor(sf::Color(c.r, c.g, c.b, c.a));

            auto rawBounds = t->getBounds();
            Bounds bounds;
            bounds.position = rawBounds.position.mul(s);
            bounds.size     = rawBounds.size.mul(s);
            auto textRect = sfText.getLocalBounds();
            Align align   = t->getModifier().getAlign();

            float x, y;
            if      (hasAlign(align, Align::CENTER_X))
                x = bounds.position.x + (bounds.size.x - textRect.size.x) * 0.5f - textRect.position.x;
            else if (hasAlign(align, Align::RIGHT))
                x = bounds.right() - textRect.size.x - textRect.position.x;
            else
                x = bounds.position.x - textRect.position.x;

            if      (hasAlign(align, Align::CENTER_Y))
                y = bounds.position.y + (bounds.size.y - textRect.size.y) * 0.5f - textRect.position.y;
            else if (hasAlign(align, Align::BOTTOM))
                y = bounds.bottom() - textRect.size.y - textRect.position.y;
            else
                y = bounds.position.y - textRect.position.y;

            sfText.setPosition({x, y});
            m_window.draw(sfText);
        });

        setDrawImage([this](Image* img) {
            const auto& pixels = img->getPixels();
            uint32_t nw = img->getNativeWidth();
            uint32_t nh = img->getNativeHeight();
            if (pixels.empty() || nw == 0 || nh == 0) return;

            const float s = m_renderScale;
            sf::Image sfImg({nw, nh}, pixels.data());
            sf::Texture sfTex(sfImg);
            sfTex.setSmooth(true);

            auto rawBounds = img->getBounds();
            Bounds bounds;
            bounds.position = rawBounds.position.mul(s);
            bounds.size     = rawBounds.size.mul(s);

            sf::Sprite sprite(sfTex);
            sprite.setScale({bounds.size.x / nw, bounds.size.y / nh});

            Align align = img->getModifier().getAlign();
            float dw = bounds.size.x;
            float dh = bounds.size.y;

            float x, y;
            if      (hasAlign(align, Align::CENTER_X))
                x = bounds.position.x + (bounds.size.x - dw) * 0.5f;
            else if (hasAlign(align, Align::RIGHT))
                x = bounds.right() - dw;
            else
                x = bounds.position.x;

            if      (hasAlign(align, Align::CENTER_Y))
                y = bounds.position.y + (bounds.size.y - dh) * 0.5f;
            else if (hasAlign(align, Align::BOTTOM))
                y = bounds.bottom() - dh;
            else
                y = bounds.position.y;

            sprite.setPosition({x, y});
            m_window.draw(sprite);
        });

        // --- Clip ---
        setPushClipRect([this](const Bounds& b) {
            m_clipStack.push_back({computeScissor(b), false, b, 0.f});
            updateGLState();
        });
        setPushClipRoundedRect([this](const Bounds& b, float r) {
            auto va = vaFromBounds(b, r);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
            m_window.draw(va);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            ++m_stencilDepth;
            m_clipStack.push_back({computeScissor(b), true, b, r});
            updateGLState();
        });
        setPopClip([this]() {
            if (m_clipStack.empty()) return;
            const auto& top = m_clipStack.back();
            if (top.isRounded) {
                auto va = vaFromBounds(top.bounds, top.radius);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glEnable(GL_STENCIL_TEST);
                glStencilFunc(GL_ALWAYS, 0, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
                m_window.draw(va);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                --m_stencilDepth;
            }
            m_clipStack.pop_back();
            updateGLState();
        });

        // --- Frame reset (called by UILO::render before drawing) ---
        setBeginFrame([this]() {
            m_clipStack.clear();
            m_stencilDepth = 0;
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_STENCIL_TEST);
            glClear(GL_STENCIL_BUFFER_BIT);
        });
    }

    void drawRectVA(Rect* r) {
        const auto& verts   = r->getVertices();
        const auto& indices = r->getIndices();
        sf::VertexArray va(sf::PrimitiveType::Triangles, indices.size());
        for (size_t i = 0; i < indices.size(); i++) {
            const auto& v = verts[indices[i]];
            va[i].position = {v.position.x, v.position.y};
            va[i].color    = sf::Color(v.color.r, v.color.g, v.color.b, v.color.a);
        }
        m_window.draw(va);
    }
};

} // namespace uilo
