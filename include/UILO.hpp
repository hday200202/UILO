#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

#include "Elements.hpp"
#include "Palette.hpp"
#include "input/Keybinds.hpp"
#include "input/Mousebinds.hpp"
#include "../include/renderer/Renderer.hpp"

namespace uilo {

class Interactible;

/*
    UILO:
    - Desc: Top-level UI controller. Owns pages and the element pool,
            drives per-frame layout and input dispatch, and manages
            overlays, floating elements, scaling, cursors, and shared
            scroll/zoom links.
*/
class UILO {
public:
    UILO() = default;
    UILO(Renderer& renderer, Page* page);

    void update();
    void render();

    // Drains the SDL queue, forwarding everything to handleEvent() and closing
    // the app on SDL_EVENT_QUIT. Convenience for the common loop:
    //     while (ui.isRunning()) { ui.pollEvents(); ui.update(); /* draw */ }
    // Applications that own their own event loop can keep calling handleEvent()
    // per event instead and never touch this.
    void pollEvents();
    bool isRunning() const { return m_running; }
    void quit()            { m_running = false; }

    void handleEvent(const SDL_Event& event);
    void dispatchScroll(const Vec2f& pos, Vec2f delta, bool precise, bool momentum = false);
    void dispatchZoom(const Vec2f& pos, float magnification);
    bool isSDLScrollTarget(const Vec2f& pos) const;

    void setRenderer(Renderer& renderer) {
        m_renderer = &renderer;
        m_mousebinds.setWindow(renderer.sdlWindow());
    }
    void addPage(Page* page);
    void setPage(const std::string& pageName);
    void setActivePage(Page* page);
    void setScale(float scale);

    void registerOverlay(Element* e, std::function<void()> onDismiss = {});
    void unregisterOverlay(Element* e);

    Element* addFloating(FreeElement f);
    void removeFloating(Element* e);

    void setCurrInteractible(Interactible* i);
    Interactible* getCurrInteractible() const { return m_currInteractible; }
    
    // Action-based input, polled once per frame from update(). Keyboard actions
    // are suppressed while a focused widget is consuming text input, so typing
    // in a Textbox never fires application shortcuts.
    Keybinds&         getKeybinds()         { return m_keybinds; }
    const Keybinds&   getKeybinds()   const { return m_keybinds; }
    Mousebinds&       getMousebinds()       { return m_mousebinds; }
    const Mousebinds& getMousebinds() const { return m_mousebinds; }

    float getScale()                const { return m_scale; }
    float getDeltaTime()            const { return m_deltaTime; }
    // Instantaneous rate for the frame just measured.
    float getFrameRate()            const { return m_deltaTime > 0.f ? 1.f / m_deltaTime : 0.f; }
    // Frames divided by elapsed time over a sampling window, refreshed every
    // getFrameRateWindow() seconds. Steadier than 1/dt, which a single long
    // frame throws off badly.
    float getAvgFrameRate()         const { return m_avgFrameRate; }
    void  setFrameRateWindow(float seconds) { m_avgFrameWindow = seconds > 0.f ? seconds : 0.25f; }
    float getFrameRateWindow()      const { return m_avgFrameWindow; }
    Vec2u getWindowSize()           const { return m_renderer ? m_renderer->getSize() : Vec2u{}; }
    Vec2f getMousePosition()        const { return m_mousePos; }
    bool isMomentumScrolling()      const { return m_inMomentumScroll; }
    bool isForcingTreeUpdate()      const { return m_forceTreeUpdate; }
    Renderer& getRenderer()         { return *m_renderer; }


    void setPalette(const Palette& palette)     { m_palette = palette; }
    void setPalette(Palette&& palette)          { m_palette = std::move(palette); }
    Palette& getPalette()                       { return m_palette; }
    const Palette& getPalette()                 const { return m_palette; }

    void requestCursor(CursorType type, int priority = 0);

    float getScrollLinkOffset(const std::string& linkId, bool horizontal) const;
    void  setScrollLinkOffset(const std::string& linkId, float offset, bool horizontal);
    float getZoomLinkValue(const std::string& linkId, bool horizontal) const;
    void  setZoomLinkValue(const std::string& linkId, float zoom, bool horizontal);

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
    bool  m_running = true;

    Timer m_timer;

    Keybinds   m_keybinds;
    Mousebinds m_mousebinds;

    Timer m_avgFrameTimer;
    int   m_avgFrameCount  = 0;
    float m_avgFrameWindow = 0.25f;
    float m_avgFrameRate   = 0.f;

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

// The web backend. Included last so every UILO declaration above is complete by
// the time it is parsed, and only when the UILO_WT build is active -- a normal
// desktop build never sees it, and never links Wt.
#ifdef UILO_WT
#include "wt/UiloWt.hpp"
#endif