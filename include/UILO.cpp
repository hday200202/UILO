#include "UILO.hpp"
#include "elements/interactible/Interactible.hpp"
#include "platform/MacScroll.hpp"
#include "platform/MacWindow.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

namespace uilo {

/*
    UILO(Renderer& renderer, Page* page):
    - Params:   Renderer& renderer, Page* page
    - Returns:  UILO
    - Desc:     Set's UILO's renderer reference, adds the Page to UILO, sets
                active page
*/
UILO::UILO(Renderer& renderer, Page* page) {
    m_renderer = &renderer;
    addPage(page);
    setPage(page->m_name);
}


/*
    addPage(Page* page):
    - Params:   Page* page
    - Returns:  void
    - Desc:     Registers a page with UILO, taking ownership and binding it to
                this instance. Keyed by the page's name.
*/
void UILO::addPage(Page* page) {
    page->setUILO(*this);
    m_pages[page->m_name] = std::unique_ptr<Page>(page);
}


/*
    setPage(const std::string& pageName):
    - Params:   const std::string& pageName
    - Returns:  void
    - Desc:     Makes the named page active, clearing overlays, resizers, popup
                backdrops, and the current interactible.
*/
void UILO::setPage(const std::string& pageName) {
    auto it = m_pages.find(pageName);
    if (it != m_pages.end()) {
        m_overlays.clear();
        m_resizers.clear();
        m_backdrops.clear();
        setCurrInteractible(nullptr);
        /* The container a coast belongs to is not on screen any more. */
        cancelMacScrollMomentum();
        m_hasScrollOrigin = false;
        m_activePage = it->second.get();
    }
}


/*
    setActivePage(Page* page):
    - Params:   Page* page
    - Returns:  void
    - Desc:     Sets the active page without taking ownership. No-op when the
                page is already active so per-frame calls don't reset the
                overlay, backdrop, and interactible state.
*/
void UILO::setActivePage(Page* page) {
    if (m_activePage == page) return;
    if (page) page->setUILO(*this);
    m_overlays.clear();
    m_resizers.clear();
    m_backdrops.clear();
    setCurrInteractible(nullptr);
    /* The container a coast belongs to is not on screen any more. */
    cancelMacScrollMomentum();
    m_hasScrollOrigin = false;
    m_activePage = page;
}


/*
    setCurrInteractible(Interactible* i):
    - Params:   Interactible* i
    - Returns:  void
    - Desc:     Sets the focused interactible, deactivating the previous one,
                and starts or stops SDL text input depending on whether the new
                interactible wants keyboard text.
*/
void UILO::setCurrInteractible(Interactible* i) {
    m_interactibleActivatedThisFrame = true;
    if (m_currInteractible == i) return;
    if (m_currInteractible) m_currInteractible->onDeactivate();
    m_currInteractible = i;

    if (m_renderer && m_renderer->sdlWindow()) {
        SDL_Window* w = m_renderer->sdlWindow();
        if (i && i->wantsTextInput()) {
            if (!SDL_TextInputActive(w)) SDL_StartTextInput(w);
        } else {
            if (SDL_TextInputActive(w))  SDL_StopTextInput(w);
        }
    }
}


/*
    setScale(float scale):
    - Params:   float scale
    - Returns:  void
    - Desc:     Sets the global UI scale factor. Ignores non-positive values.
*/
void UILO::setScale(float scale) { if (scale > 0.f) m_scale = scale; }


/*
    registerOverlay(Element* e, std::function<void()> onDismiss):
    - Params:   Element* e, std::function<void()> onDismiss
    - Returns:  void
    - Desc:     Registers an element as a modal overlay with an optional dismiss
                callback. Ignores duplicates.
*/
void UILO::registerOverlay(Element* e, std::function<void()> onDismiss) {
    for (auto& ov : m_overlays)
        if (ov.element == e) return;
    m_overlays.push_back({e, std::move(onDismiss)});
}


/*
    unregisterOverlay(Element* e):
    - Params:   Element* e
    - Returns:  void
    - Desc:     Removes an element from the overlay list.
*/
void UILO::unregisterOverlay(Element* e) {
    m_overlays.erase(
        std::remove_if(m_overlays.begin(), m_overlays.end(),
            [e](const OverlayEntry& ov) { return ov.element == e; }),
        m_overlays.end()
    );
}


/*
    addPopupBackdrop(Element* e):
    - Params:   Element* e
    - Returns:  Element*
    - Desc:     Registers a full-window popup backdrop: laid out at window size
                every frame and drawn above the page, which is how a popup
                escapes the container its owner sits in. Not the way to float an
                ordinary element -- Modifier::setFloating keeps that one in the
                tree, where it belongs.
*/
Element* UILO::addPopupBackdrop(Element* e) {
    if (!e) return nullptr;
    e->setUILO(*this);
    m_backdrops.push_back(e);
    return e;
}


/*
    removePopupBackdrop(Element* e):
    - Params:   Element* e
    - Returns:  void
    - Desc:     Removes a popup backdrop by pointer, which is what dismissing a
                popup does.
*/
void UILO::removePopupBackdrop(Element* e) {
    m_backdrops.erase(std::remove(m_backdrops.begin(), m_backdrops.end(), e),
                      m_backdrops.end());
}


/*
    requestCursor(CursorType type, int priority):
    - Params:   CursorType type, int priority
    - Returns:  void
    - Desc:     Requests a cursor for this frame. The highest-priority request
                wins; ties keep the most recent.
*/
void UILO::requestCursor(CursorType type, int priority) {
    if (priority >= m_pendingCursorPriority) {
        m_pendingCursor         = type;
        m_pendingCursorPriority = priority;
    }
}


/*
    getScrollLinkOffset(const std::string& linkId, bool horizontal):
    - Params:   const std::string& linkId, bool horizontal
    - Returns:  float
    - Desc:     Returns the shared scroll offset for a link id on the given
                axis, or 0 when unset.
*/
float UILO::getScrollLinkOffset(const std::string& linkId, bool horizontal) const {
    if (linkId.empty()) return 0.f;
    const auto& links = horizontal ? m_scrollLinksX : m_scrollLinksY;
    auto it = links.find(linkId);
    return it != links.end() ? it->second : 0.f;
}


/*
    setScrollLinkOffset(const std::string& linkId, float offset, bool horizontal):
    - Params:   const std::string& linkId, float offset, bool horizontal
    - Returns:  void
    - Desc:     Stores a shared scroll offset for a link id on the given axis so
                linked containers scroll together.
*/
void UILO::setScrollLinkOffset(const std::string& linkId, float offset, bool horizontal) {
    if (linkId.empty()) return;
    auto& links = horizontal ? m_scrollLinksX : m_scrollLinksY;
    links[linkId] = offset;
}


/*
    getZoomLinkValue(const std::string& linkId, bool horizontal):
    - Params:   const std::string& linkId, bool horizontal
    - Returns:  float
    - Desc:     Returns the shared zoom value for a link id on the given axis,
                or 1 when unset.
*/
float UILO::getZoomLinkValue(const std::string& linkId, bool horizontal) const {
    if (linkId.empty()) return 1.f;
    const auto& links = horizontal ? m_zoomLinksX : m_zoomLinksY;
    auto it = links.find(linkId);
    return it != links.end() ? it->second : 1.f;
}


/*
    setZoomLinkValue(const std::string& linkId, float zoom, bool horizontal):
    - Params:   const std::string& linkId, float zoom, bool horizontal
    - Returns:  void
    - Desc:     Stores a shared zoom value for a link id on the given axis so
                linked containers zoom together.
*/
void UILO::setZoomLinkValue(const std::string& linkId, float zoom, bool horizontal) {
    if (linkId.empty()) return;
    auto& links = horizontal ? m_zoomLinksX : m_zoomLinksY;
    links[linkId] = zoom;
}


/*
    setOnLiveResize(std::function<void()> cb):
    - Params:   std::function<void()> cb
    - Returns:  void
    - Desc:     Sets a callback invoked during native live window resize so the
                host can redraw at each intermediate size.
*/
void UILO::setOnLiveResize(std::function<void()> cb) {
    m_onLiveResize = std::move(cb);
}


/*
    update():
    - Params:   none
    - Returns:  void
    - Desc:     Advances the UI by one frame. On the first frame -- and only
                when UILO owns its window, never when embedded in a host -- it
                installs the macOS native scroll and zoom monitors and the live-
                resize configuration. Each frame it converts the SDL mouse
                position from logical points to backing pixels, updates layout
                for the active page and every floating element, culls elements
                marked for deletion, then dispatches hover, left-click, and
                right-click input. Floating elements are opaque to input: the
                topmost one under the cursor consumes hover and clicks so the
                page beneath is shielded, and draggable ones follow the cursor
                while held.
*/
void UILO::update() {
    static bool s_macInstalled = false;
    if (!s_macInstalled && m_renderer && m_renderer->ownsContext()) {
        s_macInstalled = true;
        if (m_renderer) {
            if (SDL_Window* w = m_renderer->sdlWindow()) {
                void* ns = SDL_GetPointerProperty(
                    SDL_GetWindowProperties(w),
                    SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
                configureMacWindowForLiveResize(ns);
            }
        }
        SDL_AddEventWatch(+[](void* userdata, SDL_Event* ev) -> bool {
            auto* self = static_cast<UILO*>(userdata);
            if (!self->m_onLiveResize) return true;
            if (ev->type == SDL_EVENT_WINDOW_RESIZED ||
                ev->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                self->m_onLiveResize();
            }
            return true;
        }, this);
        installMacScrollMonitor([this](float dyLines, float dxLines, bool momentum) -> bool {
            /* A coast belongs to whatever the gesture was flicked on, so
               momentum ticks replay at the position the gesture started from. */
            Vec2f pos;
            if (momentum && m_hasScrollOrigin) {
                pos = m_scrollOrigin;
            } else {
                pos               = queryMousePixelPosition();
                m_scrollOrigin    = pos;
                m_hasScrollOrigin = true;
            }
            dispatchScroll(pos, Vec2f{dxLines, dyLines}, true, momentum);
            return wantsScrollMomentum(pos);
        });
        installMacZoomMonitor([this](float mag) -> bool {
            dispatchZoom(queryMousePixelPosition(), mag);
            return true;
        });
    }

    m_deltaTime = m_timer.restart();
    tickMacScrollMomentum(m_deltaTime);
    m_pendingCursor         = CursorType::Arrow;
    m_pendingCursorPriority = 0;

    /* Averaged over a window rather than smoothing 1/dt: a single long frame
       (a stall, a resize) would otherwise drag the reported rate for a while. */
    ++m_avgFrameCount;
    const float avgWindow = m_avgFrameTimer.elapsed();
    if (avgWindow >= m_avgFrameWindow) {
        m_avgFrameRate  = static_cast<float>(m_avgFrameCount) / avgWindow;
        m_avgFrameCount = 0;
        m_avgFrameTimer.restart();
    }

    /* Action bindings run before layout so a handler that changes scale, swaps
       pages, or hides an element is reflected in this frame's tree walk. */
    const bool keyboardOwnedByWidget =
        m_currInteractible && m_currInteractible->wantsTextInput();
    m_keybinds.update(!keyboardOwnedByWidget);
    m_mousebinds.update();

    if (!m_activePage) return;

    const Vec2u windowSize = m_renderer->getSize();
    if (windowSize != m_prevWindowSize) {
        for (auto& e : m_elementPool) e->m_dirty = true;
        m_forceTreeUpdate = true;
        m_prevWindowSize = windowSize;
    } else {
        m_forceTreeUpdate = false;
    }

    Rectf logicalBounds = {
        { 0.f, 0.f },
        { static_cast<float>(windowSize.x), static_cast<float>(windowSize.y) }
    };

    m_activePage->update(logicalBounds, m_deltaTime);

    /* A backdrop covers the window, which is what makes the popup it belongs to
       modal. Its own children position themselves within it as usual. */
    const float winW = static_cast<float>(windowSize.x);
    const float winH = static_cast<float>(windowSize.y);
    for (Element* backdrop : m_backdrops) {
        Rectf slot = { {0.f, 0.f}, {winW, winH} };
        backdrop->tick(slot, m_deltaTime);
    }

    m_resizers.clear();
    m_activePage->m_rootContainer->collectResizers(m_resizers);

    m_elementPool.erase(
        std::remove_if(
            m_elementPool.begin(), m_elementPool.end(),
            [&](const std::unique_ptr<Element>& e) {
                if (e->m_markedForDeletion) {
                    if (!e->m_name.empty()) m_elements.erase(e->m_name);
                    return true;
                }
                return false;
            }
        ), m_elementPool.end()
    );

    m_mousePos = queryMousePixelPosition();
    const Vec2f mouse = m_mousePos;

    if (m_renderer) m_renderer->setMouseState(mouse);

    bool leftDown  = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))  != 0;
    bool rightDown = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;

    auto* root = m_activePage->m_rootContainer;

    /* Topmost backdrop under the pointer. A backdrop covers the window, so it
       shields the page beneath it -- which is what makes a popup modal. Held as
       an element rather than an index because a handler may dismiss the popup,
       and with it the entry, while this dispatch is still running. */
    Element* hoveredBackdrop = nullptr;
    for (auto it = m_backdrops.rbegin(); it != m_backdrops.rend(); ++it) {
        if (!(*it)->getModifier().getVisible()) continue;
        if ((*it)->getBounds().contains(mouse)) { hoveredBackdrop = *it; break; }
    }

    const bool backdropConsumedClick = hoveredBackdrop != nullptr;

    /* A resizer drag owns the pointer. Everything else is told the cursor is
       nowhere, so an element under it neither lights up nor keeps a hover it
       entered before the drag began -- and its exit handler still fires. */
    Element* draggingResizer = nullptr;
    for (auto* r : m_resizers) {
        if (static_cast<Resizer*>(r)->isDragging()) { draggingResizer = r; break; }
    }

    if (draggingResizer) {
        static constexpr Vec2f kNowhere{-1e9f, -1e9f};
        for (auto* r : m_resizers)
            if (r != draggingResizer) r->checkHover(kNowhere);
        draggingResizer->checkHover(mouse);
        for (auto& ov : m_overlays) ov.element->checkHover(kNowhere);
        if (hoveredBackdrop) hoveredBackdrop->checkHover(kNowhere);
        root->checkHover(kNowhere);
    } else {
        for (auto* r : m_resizers) r->checkHover(mouse);
        Element* hoveredOverlay = nullptr;
        for (auto& ov : m_overlays)
            if (ov.element->getBounds().contains(mouse)) { hoveredOverlay = ov.element; break; }
        if (hoveredOverlay)      hoveredOverlay->checkHover(mouse);
        else if (hoveredBackdrop) hoveredBackdrop->checkHover(mouse);
        else                     root->checkHover(mouse);
    }

    if (m_pendingCursor != m_activeCursor) {
        m_renderer->setCursor(m_pendingCursor);
        m_activeCursor = m_pendingCursor;
    }

    if (leftDown && !m_prevLeftMouse && !backdropConsumedClick) {
        m_interactibleActivatedThisFrame = false;

        bool resizerClicked = false;
        for (auto* r : m_resizers) {
            if (r->getBounds().contains(mouse)) {
                r->checkLeftClick(mouse);
                resizerClicked = true;
                break;
            }
        }

        if (!resizerClicked) {
            Element* clickedOverlay = nullptr;
            for (auto& ov : m_overlays)
                if (ov.element->getBounds().contains(mouse)) { clickedOverlay = ov.element; break; }

            if (clickedOverlay) clickedOverlay->checkLeftClick(mouse);
            else {
                auto copy = m_overlays;
                m_overlays.clear();
                for (auto& ov : copy) if (ov.onDismiss) ov.onDismiss();
                root->checkLeftClick(mouse);
                if (!m_interactibleActivatedThisFrame) setCurrInteractible(nullptr);
            }
        }
    } else if (leftDown && !m_prevLeftMouse && hoveredBackdrop) {
        m_interactibleActivatedThisFrame = false;
        hoveredBackdrop->checkLeftClick(mouse);
    }
    if (rightDown && !m_prevRightMouse) {
        m_interactibleActivatedThisFrame = false;
        if (!m_overlays.empty()) {
            auto copy = m_overlays;
            m_overlays.clear();
            for (auto& ov : copy) if (ov.onDismiss) ov.onDismiss();
        }
        if (hoveredBackdrop) hoveredBackdrop->checkRightClick(mouse);
        else                        root->checkRightClick(mouse);
        if (!m_interactibleActivatedThisFrame) setCurrInteractible(nullptr);
    }

    m_prevLeftMouse  = leftDown;
    m_prevRightMouse = rightDown;

    m_forceTreeUpdate = false;
}


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the active page, then floating elements, overlays, and
                resizers in back-to-front order.
*/
void UILO::render() {
    if (!m_activePage) return;

    m_activePage->render();
    for (Element* backdrop : m_backdrops) backdrop->render();
    for (auto& ov : m_overlays) ov.element->render();
    for (auto* r : m_resizers)  r->render();
}


/*
    queryMousePixelPosition():
    - Params:   none
    - Returns:  Vec2f
    - Desc:     The cursor in render pixels. SDL reports it in window points, so
                the ratio between the window's point and pixel size converts it;
                on a retina display those differ by the backing scale.
*/
Vec2f UILO::queryMousePixelPosition() const {
    float mx = 0.f, my = 0.f;
    SDL_GetMouseState(&mx, &my);
    if (m_renderer) if (SDL_Window* w = m_renderer->sdlWindow()) {
        int lw = 1, lh = 1, pw = 1, ph = 1;
        SDL_GetWindowSize(w, &lw, &lh);
        SDL_GetWindowSizeInPixels(w, &pw, &ph);
        if (lw > 0) mx *= (float)pw / (float)lw;
        if (lh > 0) my *= (float)ph / (float)lh;
    }
    return { mx, my };
}


/*
    dispatchScroll(const Vec2f& pos, Vec2f delta, bool precise, bool momentum):
    - Params:   const Vec2f& pos, Vec2f delta, bool precise, bool momentum
    - Returns:  void
    - Desc:     Routes a scroll delta at a position to the topmost overlay under
                it, or the active page's root otherwise. For a real gesture the
                cached cursor is refreshed first, so callbacks reading
                getMousePosition() see the position that triggered the scroll.
                Momentum ticks leave it alone: they replay at the gesture's
                origin, which is no longer where the pointer is.
*/
void UILO::dispatchScroll(const Vec2f& pos, Vec2f delta, bool precise, bool momentum) {
    if (!m_activePage || (delta.x == 0.f && delta.y == 0.f)) return;
    if (!momentum) m_mousePos = pos;
    m_inMomentumScroll = momentum;
    Element* scrollOverlay = nullptr;
    for (auto& ov : m_overlays)
        if (ov.element->getBounds().contains(pos)) { scrollOverlay = ov.element; break; }
    if (scrollOverlay) scrollOverlay->checkScroll(pos, delta, precise, momentum);
    else               m_activePage->m_rootContainer->checkScroll(pos, delta, precise, momentum);
    m_inMomentumScroll = false;
}


/*
    dispatchZoom(const Vec2f& pos, float magnification):
    - Params:   const Vec2f& pos, float magnification
    - Returns:  void
    - Desc:     Routes a zoom magnification at a position to the topmost overlay
                under it, or the active page's root otherwise.
*/
void UILO::dispatchZoom(const Vec2f& pos, float magnification) {
    if (!m_activePage || magnification == 0.f) return;
    m_mousePos = pos;
    Element* overlay = nullptr;
    for (auto& ov : m_overlays)
        if (ov.element->getBounds().contains(pos)) { overlay = ov.element; break; }
    if (overlay) overlay->checkZoom(pos, magnification);
    else         m_activePage->m_rootContainer->checkZoom(pos, magnification);
}


/*
    wantsScrollMomentum(const Vec2f& pos):
    - Params:   const Vec2f& pos
    - Returns:  bool
    - Desc:     Walks the visible element tree from the topmost overlay (or the
                page root) down to the deepest element containing the position,
                and asks that element whether a flick over it should coast. The
                macOS monitor uses this both to decide whether to start a coast
                at the end of a gesture and to stop one that has drifted onto
                something that does not want it.
    - Nothing here routes events to SDL: the monitor consumes every precise
      scroll it sees, and a mouse wheel never reaches it in the first place.
*/
bool UILO::wantsScrollMomentum(const Vec2f& pos) const {
    auto hit = [&](Element* root) -> Element* {
        Element* cur = root;
        while (cur) {
            Container* c = dynamic_cast<Container*>(cur);
            if (!c) return cur;
            Element* next = nullptr;
            for (auto* child : c->getChildren())
                if (child->getBounds().contains(pos)) next = child;
            if (!next) return cur;
            cur = next;
        }
        return nullptr;
    };

    Element* root = nullptr;
    for (auto& ov : m_overlays)
        if (ov.element->getBounds().contains(pos)) { root = ov.element; break; }
    if (!root && m_activePage) root = m_activePage->m_rootContainer;
    if (!root) return false;

    Element* deepest = hit(root);
    return deepest ? deepest->wantsScrollMomentum() : true;
}


/*
    handleEvent(const SDL_Event& event):
    - Params:   const SDL_Event& event
    - Returns:  void
    - Desc:     Processes one SDL event. Mouse-wheel events map to zoom when
                Ctrl/Cmd is held, horizontal scroll when Shift is held, or
                normal scroll otherwise. Text input and key input route to the
                focused interactible, with a filter that drops the stale key-
                repeat events Wayland can deliver after a key is released. UTF-8
                text is decoded one codepoint at a time so batched or IME input
                is not dropped.
*/
void UILO::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) m_running = false;
        handleEvent(event);
    }
}

/*
    handleEvent(const SDL_Event& event):
    - Params:   const SDL_Event& event
    - Returns:  void
    - Desc:     Turns one SDL event into UILO's own dispatch. Scroll and zoom
                gestures are separated here -- a wheel with a modifier held is a
                zoom, not a scroll -- and keyboard events are routed to the
                active interactible, except for the zoom shortcuts and the
                navigation keys, which drive scrolling when nothing has focus. A
                key repeat arriving after every key has physically come up is
                dropped, which is what stops a held key from continuing to fire
                once the window has lost and regained focus.
*/
void UILO::handleEvent(const SDL_Event& event) {
    if (!m_activePage) return;

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        const float dy = event.wheel.y;
        const float dx = event.wheel.x;
        const SDL_Keymod mods = SDL_GetModState();
        const bool shiftHeld = (mods & SDL_KMOD_SHIFT) != 0;
        const bool zoomShortcut = (mods & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
        if (zoomShortcut && dy != 0.f) {
            const float mag = dy * 0.1f;
            dispatchZoom(m_mousePos, mag);
            return;
        }
        if (shiftHeld && dx == 0.f && dy != 0.f) {
            const bool precise = std::fabs(dy - std::round(dy)) > 1e-4f
                              || (dy != 0.f && std::fabs(dy) < 1.f);
            dispatchScroll(m_mousePos, Vec2f{dy, 0.f}, precise);
            return;
        }
        const bool precise = std::fabs(dy - std::round(dy)) > 1e-4f
                          || (dy != 0.f && std::fabs(dy) < 1.f);
        dispatchScroll(m_mousePos, Vec2f{dx, dy}, precise);
    }

    if (event.type == SDL_EVENT_KEY_UP) {
        m_lastKeyUpNs = event.common.timestamp;
    }

    if (event.type == SDL_EVENT_TEXT_INPUT)
        if (m_currInteractible) {
            if (event.common.timestamp > m_lastKeyUpNs) {
                int nKeys = 0;
                const bool* state = SDL_GetKeyboardState(&nKeys);
                bool anyDown = false;
                for (int i = 0; i < nKeys; ++i) {
                    if (state[i]) { anyDown = true; break; }
                }
                if (!anyDown) return;
            }

            const unsigned char* s = reinterpret_cast<const unsigned char*>(event.text.text);
            while (*s) {
                uint32_t cp = 0;
                int n = 1;
                if ((*s & 0x80) == 0)         { cp = *s; n = 1; }
                else if ((*s & 0xE0) == 0xC0) { cp = ((*s & 0x1F) << 6)  | (s[1] & 0x3F); n = 2; }
                else if ((*s & 0xF0) == 0xE0) { cp = ((*s & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); n = 3; }
                else if ((*s & 0xF8) == 0xF0) { cp = ((*s & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F); n = 4; }
                else { ++s; continue; }
                m_currInteractible->handleTextInput(cp);
                s += n;
            }
        }

    if (event.type == SDL_EVENT_KEY_DOWN) {
        const SDL_Keymod mods = SDL_GetModState();
        const bool zoomShortcut = (mods & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
        if (zoomShortcut) {
            if (event.key.key == SDLK_EQUALS || event.key.key == SDLK_KP_PLUS) {
                dispatchZoom(m_mousePos, 0.1f);
                return;
            }
            if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS) {
                dispatchZoom(m_mousePos, -0.1f);
                return;
            }
        }

        if ((event.key.key == SDLK_UP || event.key.key == SDLK_DOWN ||
             event.key.key == SDLK_PAGEUP || event.key.key == SDLK_PAGEDOWN ||
             event.key.key == SDLK_HOME || event.key.key == SDLK_END) &&
            !m_currInteractible) {
            const bool shiftHeld = (mods & SDL_KMOD_SHIFT) != 0;
            float vertical = 0.f;
            float horizontal = 0.f;
            if (event.key.key == SDLK_UP) vertical = -1.f;
            else if (event.key.key == SDLK_DOWN) vertical = 1.f;
            else if (event.key.key == SDLK_PAGEUP) vertical = -3.f;
            else if (event.key.key == SDLK_PAGEDOWN) vertical = 3.f;
            else if (event.key.key == SDLK_HOME) vertical = -1000.f;
            else if (event.key.key == SDLK_END) vertical = 1000.f;

            if (shiftHeld) {
                horizontal = vertical;
                vertical = 0.f;
            }

            if (horizontal != 0.f || vertical != 0.f) {
                dispatchScroll(m_mousePos, Vec2f{horizontal, vertical}, false);
                return;
            }
        }

        if (!m_currInteractible) return;

        if (event.key.repeat && event.common.timestamp > m_lastKeyUpNs) {
            int nKeys = 0;
            const bool* state = SDL_GetKeyboardState(&nKeys);
            bool anyDown = false;
            for (int i = 0; i < nKeys; ++i) {
                if (state[i]) { anyDown = true; break; }
            }
            if (!anyDown) return;
        }

        bool shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
        bool ctrl  = (event.key.mod & SDL_KMOD_CTRL)  != 0;
        bool gui   = (event.key.mod & SDL_KMOD_GUI)   != 0;
        m_currInteractible->handleKeyInput(event.key.key, shift, ctrl, gui);
    }
}

}
