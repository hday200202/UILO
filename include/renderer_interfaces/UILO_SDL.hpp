#pragma once

#define GL_SILENCE_DEPRECATION
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <vector>

#include "../graphics/Renderer.hpp"
#include "../elements/decor/Text.hpp"
#include "../elements/decor/Image.hpp"
#include "../utils/Alignment.hpp"
#include "../../assets/EmbeddedFont.hpp"

namespace uilo {

class SDLRenderer : public Renderer {
public:
    // Call these before SDL_CreateWindow so the GL context gets the right attributes.
    static void setGLAttributes() {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    }

    // window must have been created with SDL_WINDOW_OPENGL.
    // ctx should be the result of SDL_GL_CreateContext(window).
    explicit SDLRenderer(SDL_Window* window, SDL_GLContext ctx)
        : m_window(window), m_ctx(ctx)
    {
        TTF_Init();
        auto* io = SDL_IOFromConstMem(
            uilo::EMBEDDED_DEJAVUSANS_FONT.data(),
            uilo::EMBEDDED_DEJAVUSANS_FONT.size());
        m_font = TTF_OpenFontIO(io, true, 16.f); // size will be overridden per draw
        setupCallbacks();
    }

    ~SDLRenderer() {
        if (m_font) TTF_CloseFont(m_font);
    }

private:
    SDL_Window* m_window;
    [[maybe_unused]] SDL_GLContext m_ctx;
    TTF_Font* m_font = nullptr;

    struct ClipEntry { bool isRounded; Bounds bounds; float radius; };
    std::vector<ClipEntry> m_clipStack;
    int m_stencilDepth = 0;

    void drawVertices(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices) {
        glBegin(GL_TRIANGLES);
        for (uint32_t idx : indices) {
            const auto& v = verts[idx];
            glColor4ub(v.color.r, v.color.g, v.color.b, v.color.a);
            glVertex2f(v.position.x, v.position.y);
        }
        glEnd();
    }

    void drawBoundsGL(const Bounds& b, float r) {
        Rect shape;
        shape.setPosition({b.position.x, b.position.y});
        shape.setSize({b.size.x, b.size.y});
        shape.setCornerRadius(r);
        shape.setColor({255, 255, 255, 255});
        drawVertices(shape.getVertices(), shape.getIndices());
    }

    // Recompute and apply scissor + stencil test based on current clip stack.
    void updateGLClipState() {
        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);

        if (m_clipStack.empty()) {
            glDisable(GL_SCISSOR_TEST);
        } else {
            // Intersect all clip bounds to get the tightest scissor rect.
            float x1 = m_clipStack[0].bounds.position.x;
            float y1 = m_clipStack[0].bounds.position.y;
            float x2 = m_clipStack[0].bounds.position.x + m_clipStack[0].bounds.size.x;
            float y2 = m_clipStack[0].bounds.position.y + m_clipStack[0].bounds.size.y;
            for (size_t i = 1; i < m_clipStack.size(); ++i) {
                const auto& b = m_clipStack[i].bounds;
                x1 = std::max(x1, b.position.x);
                y1 = std::max(y1, b.position.y);
                x2 = std::min(x2, b.position.x + b.size.x);
                y2 = std::min(y2, b.position.y + b.size.y);
            }
            int sx1 = (int)std::floor(x1);
            int sy1 = (int)std::floor(y1);
            int sx2 = (int)std::ceil(x2);
            int sy2 = (int)std::ceil(y2);
            int sw = std::max(0, sx2 - sx1);
            int sh = std::max(0, sy2 - sy1);
            glEnable(GL_SCISSOR_TEST);
            // GL scissor origin is bottom-left; flip Y.
            glScissor(sx1, h - sy2, sw, sh);
        }

        if (m_stencilDepth == 0) {
            glDisable(GL_STENCIL_TEST);
        } else {
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_EQUAL, m_stencilDepth, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        }
    }

    void setupCallbacks() {
        setDrawFilledRect([this](Rect* r) {
            drawVertices(r->getVertices(), r->getIndices());
        });
        setDrawRoundedRect([this](Rect* r, float radius) {
            r->setCornerRadius(radius);
            drawVertices(r->getVertices(), r->getIndices());
        });
        setDrawText([this](Text* t) {
            if (!m_font) return;
            const std::string& str = t->getString();
            if (str.empty()) return;

            const float s = m_renderScale;
            TTF_SetFontSize(m_font, (float)t->getFontSize() * s);
            auto c = t->getModifier().getColor();
            SDL_Color fg{c.r, c.g, c.b, c.a};

            SDL_Surface* surf = TTF_RenderText_Blended_Wrapped(
                m_font, str.c_str(), 0, fg, 0);
            if (!surf) return;

            SDL_Surface* rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(surf);
            if (!rgba) return;

            int tw = rgba->w, th = rgba->h;

            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, rgba->pitch / 4);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            SDL_DestroySurface(rgba);

            auto rawBounds = t->getBounds();
            Bounds bounds;
            bounds.position = rawBounds.position.mul(s);
            bounds.size     = rawBounds.size.mul(s);
            Align align = t->getModifier().getAlign();

            float x, y;
            if      (hasAlign(align, Align::CENTER_X))
                x = bounds.position.x + (bounds.size.x - tw) * 0.5f;
            else if (hasAlign(align, Align::RIGHT))
                x = bounds.right() - (float)tw;
            else
                x = bounds.position.x;

            if      (hasAlign(align, Align::CENTER_Y))
                y = bounds.position.y + (bounds.size.y - th) * 0.5f;
            else if (hasAlign(align, Align::BOTTOM))
                y = bounds.bottom() - (float)th;
            else
                y = bounds.position.y;

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tex);
            glColor4f(1.f, 1.f, 1.f, 1.f);
            glBegin(GL_QUADS);
                glTexCoord2f(0.f, 0.f); glVertex2f(x,      y     );
                glTexCoord2f(1.f, 0.f); glVertex2f(x + tw, y     );
                glTexCoord2f(1.f, 1.f); glVertex2f(x + tw, y + th);
                glTexCoord2f(0.f, 1.f); glVertex2f(x,      y + th);
            glEnd();
            glDisable(GL_TEXTURE_2D);

            glDeleteTextures(1, &tex);
        });

        setDrawImage([this](Image* img) {
            const auto& pixels = img->getPixels();
            uint32_t nw = img->getNativeWidth();
            uint32_t nh = img->getNativeHeight();
            if (pixels.empty() || nw == 0 || nh == 0) return;

            const float s = m_renderScale;
            auto rawBounds = img->getBounds();
            Bounds bounds;
            bounds.position = rawBounds.position.mul(s);
            bounds.size     = rawBounds.size.mul(s);

            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nw, nh, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

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

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tex);
            glColor4f(1.f, 1.f, 1.f, 1.f);
            glBegin(GL_QUADS);
                glTexCoord2f(0.f, 0.f); glVertex2f(x,      y     );
                glTexCoord2f(1.f, 0.f); glVertex2f(x + dw, y     );
                glTexCoord2f(1.f, 1.f); glVertex2f(x + dw, y + dh);
                glTexCoord2f(0.f, 1.f); glVertex2f(x,      y + dh);
            glEnd();
            glDisable(GL_TEXTURE_2D);

            glDeleteTextures(1, &tex);
        });

        setPushClipRect([this](const Bounds& b) {
            m_clipStack.push_back({false, b, 0.f});
            updateGLClipState();
        });
        setPushClipRoundedRect([this](const Bounds& b, float r) {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
            drawBoundsGL(b, r);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            ++m_stencilDepth;
            m_clipStack.push_back({true, b, r});
            updateGLClipState();
        });
        setPopClip([this]() {
            if (m_clipStack.empty()) return;
            const auto& top = m_clipStack.back();
            if (top.isRounded) {
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glEnable(GL_STENCIL_TEST);
                glStencilFunc(GL_ALWAYS, 0, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
                drawBoundsGL(top.bounds, top.radius);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                --m_stencilDepth;
            }
            m_clipStack.pop_back();
            updateGLClipState();
        });

        setBeginFrame([this]() {
            m_clipStack.clear();
            m_stencilDepth = 0;

            // Set up an orthographic projection matching logical window size (top-left origin).
            int w, h;
            SDL_GetWindowSize(m_window, &w, &h);
            glViewport(0, 0, w, h);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0.0, (double)w, (double)h, 0.0, -1.0, 1.0);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_STENCIL_TEST);
            glClear(GL_STENCIL_BUFFER_BIT);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_MULTISAMPLE);
        });
    }
};

} // namespace uilo
