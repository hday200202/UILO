#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

#include "Elements.hpp"
#include "Palette.hpp"
#include "../include/renderer/Renderer.hpp"

namespace uilo {

class Interactible;

class UILO {
public:
    UILO() = default;
    // Takes ownership of the Renderer (call renderer.init() before passing).
    UILO(Renderer& renderer, Page* page);

    void update();
    void render();
    void handleEvent(const SDL_Event& event);
    // Routes a scroll delta to the topmost overlay under `pos`, or to the
    // active page's root container. Exposed because the macOS native scroll
    // monitor calls this from outside the SDL event loop.
    void dispatchScroll(const Vec2f& pos, Vec2f delta, bool precise, bool momentum = false);
    // Routes a pinch/zoom magnification delta (per-event ratio) to the
    // topmost element under `pos`. Called by the macOS pinch monitor;
    // also reusable for keyboard-zoom shortcuts.
    void dispatchZoom(const Vec2f& pos, float magnification);
    // True if the topmost element under `pos` is an Interactible (Slider,
    // Knob, Textbox, Button, etc.) — i.e. something that should receive
    // raw wheel events via SDL, not the macOS momentum-scroll path.
    bool isSDLScrollTarget(const Vec2f& pos) const;

    void setRenderer(Renderer& renderer) { m_renderer = &renderer; }

    void addPage(Page* page);
    void setPage(const std::string& pageName);
    void setScale(float scale);

    void registerOverlay(Element* e, std::function<void()> onDismiss = {});
    void unregisterOverlay(Element* e);

    // Floating elements live outside the page layout flow. They are owned
    // by UILO, updated + rendered every frame at a position resolved from
    // the FreeElement (which supplies position) and the element's
    // Modifier (which supplies width/height). They do not participate in
    // page hit-testing; if marked draggable they consume their own clicks.
    Element* addFloating(FreeElement f);
    void     removeFloating(Element* e);

    void setCurrInteractible(Interactible* i);
    Interactible* getCurrInteractible() const { return m_currInteractible; }
    
    float getScale()      const { return m_scale; }
    float getDeltaTime()  const { return m_deltaTime; }
    Vec2u  getWindowSize() const { return m_renderer ? m_renderer->getSize() : Vec2u{}; }
    Vec2f  getMousePosition() const { return m_mousePos; }
    // True only while a callback is being invoked from a synthesized
    // momentum-coast scroll tick (macOS trackpad). Use this to keep
    // gesture-anchored state (e.g. zoom pivots) stable across coast.
    bool   isMomentumScrolling() const { return m_inMomentumScroll; }
    // True only during the frame where UILO detected a window resize.
    // Containers/elements can use this to run a one-shot full update pass
    // even for nodes that would normally be skipped by optimizations.
    bool   isForcingTreeUpdate() const { return m_forceTreeUpdate; }
    Renderer& getRenderer() { return *m_renderer; }


    // Theming. The palette is owned by this UILO instance; elements that
    // have a non-empty, non-"none" color role read from it at draw time.
    // Default-constructed UILO has an empty palette (every role resolves
    // to the per-element literal color, preserving the old behavior).
    void setPalette(const Palette& palette) { m_palette = palette; }
    void setPalette(Palette&& palette)      { m_palette = std::move(palette); }
    Palette&       getPalette()       { return m_palette; }
    const Palette& getPalette() const { return m_palette; }
    void requestCursor(CursorType type, int priority = 0);

    float getScrollLinkOffset(const std::string& linkId, bool horizontal) const;
    void  setScrollLinkOffset(const std::string& linkId, float offset, bool horizontal);
    float getZoomLinkValue(const std::string& linkId, bool horizontal) const;
    void  setZoomLinkValue(const std::string& linkId, float zoom, bool horizontal);

    // Register a callback invoked synchronously by SDL during the macOS
    // live-resize loop (and on normal resize events). The host should do
    // a full beginFrame/clear/ui.render()/endFrame inside it so the
    // window paints during the gesture instead of leaving Cocoa to
    // stretch the last drawable. Pair with native layer setup that
    // UILO::update() applies on the first frame on macOS.
    void setOnLiveResize(std::function<void()> cb);

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

    struct FloatingEntry {
        Element*  element   = nullptr;
        Dimension xPos      = 0_px;
        Dimension yPos      = 0_px;
        bool      draggable = false;
        bool      dragging  = false;
        Vec2f     dragOffset{};
    };
    std::vector<FloatingEntry> m_floating;

    Page* m_activePage = nullptr;

    float m_scale = 1.f;
    float m_deltaTime = 0.f;

    Timer m_timer;

    Renderer* m_renderer = nullptr;
    Vec2u     m_prevWindowSize = {0u, 0u};
    Vec2f     m_mousePos       = {};
    bool      m_inMomentumScroll = false;
    bool      m_forceTreeUpdate = false;

    std::function<void()> m_onLiveResize;

    bool m_prevLeftMouse  = false;
    bool m_prevRightMouse = false;

    Palette m_palette;

    std::unordered_map<std::string, float> m_scrollLinksX;
    std::unordered_map<std::string, float> m_scrollLinksY;
    std::unordered_map<std::string, float> m_zoomLinksX;
    std::unordered_map<std::string, float> m_zoomLinksY;

    CursorType m_pendingCursor         = CursorType::Arrow;
    int        m_pendingCursorPriority = 0;
    CursorType m_activeCursor          = CursorType::Arrow;

    Interactible* m_currInteractible             = nullptr;
    bool          m_interactibleActivatedThisFrame = false;

    Uint64 m_lastKeyUpNs = 0;

    friend class Element;
    friend class Interactible;
};

}