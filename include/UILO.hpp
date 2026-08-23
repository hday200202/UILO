#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <optional>

#include "Elements.hpp"
#include "Palette.hpp"
#include "Defaults.hpp"
#include "utils/OS.hpp"
#include "utils/Resources.hpp"
#include "utils/Theme.hpp"
#include "input/Keybinds.hpp"
#include "input/Mousebinds.hpp"
#include "../include/renderer/Renderer.hpp"

namespace uilo {

class Interactible;
class UILO;

#ifdef UILO_WT
namespace wt {
/*
    WebConfig:
    - Desc: Server-side settings for UILO::runWeb -- the things that have no
            desktop equivalent because they describe the page and the HTTP
            server, not the UI. The UI itself (pages, palette, callbacks) is set
            on the UILO instance exactly as on desktop.
*/
struct WebConfig {
    std::string title       = "UILO";
    int         port        = 8080;
    std::string address     = "0.0.0.0";
    std::string docRoot     = "docroot";                       // static files, served at /
    std::string resourcesDir = "ext/UILO/ext/wt/resources";    // Wt's bundled JS/themes

    // Font mapping: UILO loads a .ttf by path, the browser needs a CSS family,
    // and charSize is a pixel height (ascent+descent) whereas CSS font-size
    // sizes the em square. Defaults are fine for most apps.
    std::string fontFamily =
        "system-ui, -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif";
    float       charSizeToFontSize = 0.86f;
};


/*
    WebApp:
    - Desc: One browser session's UILO, plus whatever else has to stay alive for
            as long as that session does. Implement it on the type that owns
            your UILO and hand a factory to the per-session runWeb: the server
            makes one per connection and destroys it when the connection ends,
            so every browser gets its own pages, palette and application state.

            The single-instance runWeb needs none of this -- there, every
            connection shares the one UILO it was called on.
*/
class WebApp {
public:
    virtual ~WebApp() = default;
    virtual UILO& ui() = 0;
};
}
#endif

/*
    UILO:
    - Desc:     The top-level controller. It owns the pages and the element
                pool, drives layout and input dispatch once per frame, and
                manages overlays, the floating layer, scaling, cursor requests
                and the shared scroll and zoom links.
    - Ownership runs through here rather than through the tree. An element binds
      itself with setUILO() on the first walk that reaches it, which puts it in
      the pool, and erase() only marks it -- the sweep happens between frames,
      so a handler is free to remove elements while the tree is still being
      walked.
    - The floating layer is the one part of the tree that is ticked, hit-tested
      and drawn above the page. A popup has to live there rather than as a
      child, or the container it sits in would clip it.
*/
class UILO {
public:
    UILO();
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
    bool wantsScrollMomentum(const Vec2f& pos) const;
    // Cursor position in render pixels, read live from SDL rather than from the
    // once-per-frame cache, so event callbacks that fire mid-frame see where the
    // pointer actually is.
    Vec2f queryMousePixelPosition() const;

    void setRenderer(Renderer& renderer);
    void addPage(Page* page);
    void setPage(const std::string& pageName);
    void setActivePage(Page* page);
    Page* getActivePage() const { return m_activePage; }
    void setScale(float scale);

#ifdef UILO_WT
    // Serves this UILO over the web (Wt) and blocks until the server stops --
    // the web counterpart of the desktop pollEvents/update/render loop. Set up
    // the UILO exactly as on desktop (addPage/setPage/setTheme/callbacks)
    // first; runWeb translates the active page, drives it from browser events,
    // and follows setPage() for navigation. `config` is server-side only.
    //
    // Every connection shares this one instance: two browsers are two views of
    // the same UI, and they are driven from different threads. Single-user
    // tools want this; anything else wants the factory form below.
    int runWeb(const wt::WebConfig& config = {});

    // Per-session form. `factory` is called once per browser connection, on
    // that connection's thread, and the WebApp it returns lives and dies with
    // the connection -- so each browser gets its own UILO and its own state,
    // and no two sessions touch the same tree.
    static int runWeb(std::function<std::unique_ptr<wt::WebApp>()> factory,
                      const wt::WebConfig& config = {});
#endif

    void registerOverlay(Element* e, std::function<void()> onDismiss = {});
    void unregisterOverlay(Element* e);

    // The one context menu, held at the root of the UI and shared by every
    // element. Created the first time it is needed, so a UI that is never
    // right-clicked never builds one. Style it through its options:
    //     ui.contextMenu()->getOptions().setItemHeight(24.f);
    ContextMenu* contextMenu();
    // Fills the shared menu from `items` and shows it at `at`. This is what
    // Modifier::setContextMenu ends up calling; an application can call it
    // directly to open a menu from something other than a right-click.
    void showContextMenu(const std::vector<ContextMenuItem>& items, const Vec2f& at);
    void closeContextMenu();
    bool isContextMenuOpen() const;

    // Popup infrastructure, not the way to float an ordinary element -- use
    // Modifier::setFloating for that. A backdrop is laid out at window size and
    // drawn above the page, which is how a Dropdown list or a DatePicker
    // calendar escapes the container it belongs to.
    Element* addPopupBackdrop(Element* e);
    void     removePopupBackdrop(Element* e);

    std::vector<Element*> getPopupBackdrops() const;

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
    // The cursor currently applied to the window, i.e. the winner of last
    // frame's requests. A host embedding UILO reads this to keep its own cursor
    // handling in step.
    CursorType getActiveCursor()    const { return m_activeCursor; }
    bool isMomentumScrolling()      const { return m_inMomentumScroll; }
    bool isForcingTreeUpdate()      const { return m_forceTreeUpdate; }
    Renderer& getRenderer()         { return *m_renderer; }
    // Whether a renderer has been set. Anything that measures text has to ask,
    // because a UILO can exist before one is bound.
    bool      hasRenderer()   const { return m_renderer != nullptr; }


    // The look of everything this UILO owns: the palette, and a prototype for
    // every Modifier and *Options type. Starts as Defaults.hpp. Setting one
    // re-applies it to every element already built, so restyling needs no
    // rebuild.
    void setTheme(const Theme& theme);
    // This UILO's theme, to change in place. Anything set through here reaches
    // elements on the next frame; call refreshTheme() to apply it at once.
    Theme&         getTheme()       { return m_theme; }
    const Theme&   getTheme() const { return m_theme; }
    const Palette& getPalette() const { return m_theme.palette(); }
    // Re-applies the current theme to every element in the pool. setTheme()
    // does this for you; call it directly after editing getTheme() in place.
    void refreshTheme();

    void requestCursor(CursorType type, int priority = 0);

    float getScrollLinkOffset(const std::string& linkId, bool horizontal) const;
    void  setScrollLinkOffset(const std::string& linkId, float offset, bool horizontal);
    float getZoomLinkValue(const std::string& linkId, bool horizontal) const;
    void  setZoomLinkValue(const std::string& linkId, float zoom, bool horizontal);

    void setOnLiveResize(std::function<void()> cb);

    template <typename T> T* getElement(const std::string& name);

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

    /* Full-window popup backdrops. Position and dragging used to live here too;
       an ordinary floating element is a normal child now, so a backdrop is just
       an element laid out against the window. */
    std::vector<Element*> m_backdrops;

    /* The shared context menu. Owned by the element pool like everything else,
       so this is a borrowed pointer; null until something asks for a menu. */
    ContextMenu* m_contextMenu = nullptr;

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
    // Where the in-progress scroll gesture started. Momentum ticks are dispatched
    // here instead of at the live cursor, so a coast keeps scrolling whatever was
    // flicked even if the pointer has since moved off it.
    Vec2f     m_scrollOrigin     = {};
    bool      m_hasScrollOrigin  = false;
    bool      m_forceTreeUpdate = false;

    std::function<void()> m_onLiveResize;

    bool m_prevLeftMouse  = false;
    bool m_prevRightMouse = false;

    /* Presses latched out of the event stream, because polling the button state
       once a frame misses a tap whose press and release both arrive in the same
       batch -- which is every two-finger tap on a trackpad. Cleared by the
       dispatch that consumes them. */
    bool  m_pendingLeftPress  = false;
    bool  m_pendingRightPress = false;
    Vec2f m_pendingPressPos   = {};

    /* Window points to render pixels, which differ by the backing scale. */
    Vec2f toPixels(float x, float y) const;

    Theme m_theme;

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


/*
    setRenderer(Renderer& renderer):
    - Params:   Renderer& renderer
    - Returns:  void
    - Desc:     Attaches the renderer this UILO draws through, and hands its SDL
                window to the mouse bindings so they can resolve positions
                against it.
*/
inline void UILO::setRenderer(Renderer& renderer) {
    m_renderer = &renderer;
    m_mousebinds.setWindow(renderer.sdlWindow());
}


/*
    getPopupBackdrops():
    - Params:   none
    - Returns:  std::vector<Element*> -- in insertion order
    - Desc:     The elements currently in the floating layer. Native rendering
                walks the internal list directly; this exists for the web
                bridge, which cannot see it and reads this on each sync to
                mirror popups -- a picker adds its backdrop here when it opens
                -- into an on-top overlay.
*/
inline std::vector<Element*> UILO::getPopupBackdrops() const {
    std::vector<Element*> out;
    out.reserve(m_backdrops.size());
    for (Element* e : m_backdrops) out.push_back(e);
    return out;
}


/*
    getElement(const std::string& name):
    - Params:   const std::string& name
    - Returns:  T* -- null when the name is unknown or the type does not match
    - Desc:     Looks up a named element and casts it to the requested type.
                Only elements given a name at construction are registered, and
                the cast is checked, so asking for the wrong type yields null
                rather than undefined behaviour.
*/
template <typename T>
T* UILO::getElement(const std::string& name) {
    auto it = m_elements.find(name);
    if (it == m_elements.end()) return nullptr;
    return dynamic_cast<T*>(it->second);
}

}

// The web backend. Included last so every UILO declaration above is complete by
// the time it is parsed, and only when the UILO_WT build is active -- a normal
// desktop build never sees it, and never links Wt.
#ifdef UILO_WT
#include "wt/UiloWt.hpp"
#endif