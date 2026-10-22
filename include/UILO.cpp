#include "UILO.hpp"
#include "elements/interactible/Interactible.hpp"
#include "platform/MacScroll.hpp"
#include "platform/MacWindow.hpp"
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
    setRenderer(renderer);
    addPage(page);
    setPage(page->m_name);
}


/*
    UILO():
    - Params:   none
    - Returns:  n/a
    - Desc:     Starts on the defaults in Defaults.hpp. A UILO owns its theme
                outright, so two of them can look different in the same process
                -- which is what a Wt session needs.
*/
UILO::UILO() {
    installDefaultTheme(m_theme);
}


/*
    setTheme(const Theme& theme):
    - Params:   const Theme& theme
    - Returns:  void
    - Desc:     Replaces the look of everything this UILO owns and re-applies it
                to every element already built, so a restyle needs no rebuild.
*/
void UILO::setTheme(const Theme& theme) {
    m_theme = theme;
    refreshTheme();
}


/*
    refreshTheme():
    - Params:   none
    - Returns:  void
    - Desc:     Pushes the current theme through every element in the pool.
                Each one takes only what its call site left alone, so this is
                safe to run repeatedly and never undoes an explicit setting.
*/
void UILO::refreshTheme() {
    for (auto& element : m_elementPool) {
        if (element) element->applyTheme(m_theme);
    }
    m_forceTreeUpdate = true;
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
                and starts or stops the window's text input depending on whether
                the new interactible wants keyboard text.
*/
void UILO::setCurrInteractible(Interactible* i) {
    m_interactibleActivatedThisFrame = true;
    if (m_currInteractible == i) return;
    if (m_currInteractible) m_currInteractible->onDeactivate();
    m_currInteractible = i;

    if (valo::Window* w = window()) {
        if (i && i->wantsTextInput()) {
            if (!w->isTextInputActive()) w->startTextInput();
        } else {
            if (w->isTextInputActive())  w->stopTextInput();
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
    contextMenu():
    - Params:   none
    - Returns:  ContextMenu*
    - Desc:     The shared menu, built on first use. It is held here rather than
                in the page tree so it survives navigation and is never clipped
                by whatever container the click landed in, and it is bound to
                this UILO immediately so it can measure text before it is first
                laid out.
*/
ContextMenu* UILO::contextMenu() {
    if (!m_contextMenu) {
        m_contextMenu = new ContextMenu(Modifier(), ContextMenuOptions(), "");
        m_contextMenu->setUILO(*this);
    }
    return m_contextMenu;
}


/*
    showContextMenu(const std::vector<ContextMenuItem>& items, const Vec2f& at):
    - Params:   const std::vector<ContextMenuItem>& items, const Vec2f& at
    - Returns:  void
    - Desc:     Fills the shared menu and opens it at a point. An empty item list
                closes whatever was open instead, so a builder that declines has
                the same effect as no menu at all.
*/
void UILO::showContextMenu(const std::vector<ContextMenuItem>& items, const Vec2f& at) {
    if (items.empty()) { closeContextMenu(); return; }
    ContextMenu* menu = contextMenu();
    menu->setItems(items);
    menu->openAt(at);
}


/*
    closeContextMenu():
    - Params:   none
    - Returns:  void
    - Desc:     Puts the menu away, submenus included. Safe before one has ever
                been built.
*/
void UILO::closeContextMenu() {
    if (m_contextMenu) m_contextMenu->close();
}


/*
    isContextMenuOpen():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether a menu is showing. Worth testing before acting on a key
                the menu also uses.
*/
bool UILO::isContextMenuOpen() const {
    return m_contextMenu && m_contextMenu->isOpen();
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
                resize configuration. Each frame it reads the mouse position
                in backing pixels, updates layout
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
        if (valo::Window* w = window()) {
            /* The NSWindow on macOS; the call is a no-op everywhere else. */
            configureMacWindowForLiveResize(w->nativeHandle());

            /* Runs inside the OS's modal resize loop too, where the main loop is
               blocked, so the host can redraw at every intermediate size. */
            w->setResizeCallback([this] {
                if (m_onLiveResize) m_onLiveResize();
            });
        }
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

    /* The menu is not in the page tree, so nothing else would lay it out. Its
       own bounds are where openAt() put it, and its submenus follow from the row
       bounds this pass resolves. */
    for (ContextMenu* menu = m_contextMenu; menu && menu->isOpen();
         menu = menu->openSubmenu()) {
        Rectf slot = menu->getBounds();
        menu->tick(slot, m_deltaTime);
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

    const bool leftHeld  = isMouseButtonDown(MouseButton::Left);
    const bool rightHeld = isMouseButtonDown(MouseButton::Right);

    /* A press counts when the polled state rises OR when one was latched out of
       the event stream. The latch is what catches a tap that was already over by
       the time this frame polled -- every trackpad two-finger tap, and any fast
       click. Its position wins, since it is where the button actually went down. */
    const bool leftPressed  = (leftHeld  && !m_prevLeftMouse)  || m_pendingLeftPress;
    const bool rightPressed = (rightHeld && !m_prevRightMouse) || m_pendingRightPress;
    const Vec2f pressPos = (m_pendingLeftPress || m_pendingRightPress)
                         ? m_pendingPressPos : mouse;
    m_pendingLeftPress  = false;
    m_pendingRightPress = false;

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

    static constexpr Vec2f kNowhere{-1e9f, -1e9f};

    if (draggingResizer) {
        for (auto* r : m_resizers)
            if (r != draggingResizer) r->checkHover(kNowhere);
        draggingResizer->checkHover(mouse);
        for (auto& ov : m_overlays) ov.element->checkHover(kNowhere);
        for (Element* backdrop : m_backdrops) backdrop->checkHover(kNowhere);
        root->checkHover(kNowhere);
    } else {
        for (auto* r : m_resizers) r->checkHover(mouse);

        /* Topmost overlay under the pointer: later registrations draw over
           earlier ones, so the search runs backwards. */
        Element* hoveredOverlay = nullptr;
        for (auto it = m_overlays.rbegin(); it != m_overlays.rend(); ++it)
            if (it->element->getBounds().contains(mouse)) { hoveredOverlay = it->element; break; }

        /* Every layer is visited, and each one the pointer is not on is told it
           is nowhere. Skipping them instead would leave a widget the pointer has
           moved off lit up underneath the popup that now covers it. */
        for (auto& ov : m_overlays)
            ov.element->checkHover(ov.element == hoveredOverlay ? mouse : kNowhere);

        const bool covered = hoveredOverlay != nullptr;
        for (Element* backdrop : m_backdrops)
            backdrop->checkHover(!covered && backdrop == hoveredBackdrop ? mouse : kNowhere);

        root->checkHover(covered || hoveredBackdrop ? kNowhere : mouse);
    }

    if (m_pendingCursor != m_activeCursor) {
        m_renderer->setCursor(m_pendingCursor);
        m_activeCursor = m_pendingCursor;
    }

    if (leftPressed && !backdropConsumedClick) {
        m_interactibleActivatedThisFrame = false;

        bool resizerClicked = false;
        for (auto* r : m_resizers) {
            if (r->getBounds().contains(pressPos)) {
                r->checkLeftClick(pressPos);
                resizerClicked = true;
                break;
            }
        }

        if (!resizerClicked) {
            Element* clickedOverlay = nullptr;
            for (auto it = m_overlays.rbegin(); it != m_overlays.rend(); ++it)
                if (it->element->getBounds().contains(pressPos)) { clickedOverlay = it->element; break; }

            if (clickedOverlay) clickedOverlay->checkLeftClick(pressPos);
            else {
                auto copy = m_overlays;
                m_overlays.clear();
                for (auto& ov : copy) if (ov.onDismiss) ov.onDismiss();
                root->checkLeftClick(pressPos);
                if (!m_interactibleActivatedThisFrame) setCurrInteractible(nullptr);
            }
        }
    } else if (leftPressed && hoveredBackdrop) {
        m_interactibleActivatedThisFrame = false;
        hoveredBackdrop->checkLeftClick(pressPos);
    }
    if (rightPressed) {
        m_interactibleActivatedThisFrame = false;
        if (!m_overlays.empty()) {
            auto copy = m_overlays;
            m_overlays.clear();
            for (auto& ov : copy) if (ov.onDismiss) ov.onDismiss();
        }
        if (hoveredBackdrop) hoveredBackdrop->checkRightClick(pressPos);
        else                        root->checkRightClick(pressPos);
        if (!m_interactibleActivatedThisFrame) setCurrInteractible(nullptr);
    }

    m_prevLeftMouse  = leftHeld;
    m_prevRightMouse = rightHeld;

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
    - Desc:     The cursor in render pixels, read live from the window. On a
                retina display that is the window-point position times the
                backing scale, which the window has already applied.
*/
Vec2f UILO::queryMousePixelPosition() const {
    if (valo::Window* w = window()) {
        const valo::Vec2f p = w->mousePosition();
        return { p.x, p.y };
    }
    return m_mousePos;
}


/*
    isMouseButtonDown(MouseButton button):
    - Params:   MouseButton button
    - Returns:  bool
    - Desc:     Whether a mouse button is physically held right now.
*/
bool UILO::isMouseButtonDown(MouseButton button) const {
    valo::Window* w = window();
    return w && w->isMouseDown(button);
}


/*
    getKeyModifiers():
    - Params:   none
    - Returns:  KeyMod
    - Desc:     The modifier keys held as of the last event read.
*/
KeyMod UILO::getKeyModifiers() const {
    valo::Window* w = window();
    return w ? w->modState() : KeyMod::None;
}


/*
    getClipboardText():
    - Params:   none
    - Returns:  std::string
    - Desc:     The system clipboard as UTF-8, empty when it holds no text.
*/
std::string UILO::getClipboardText() const {
    valo::Window* w = window();
    return w ? w->getClipboardText() : std::string{};
}


/*
    setClipboardText(const std::string& text):
    - Params:   const std::string& text
    - Returns:  void
    - Desc:     Puts UTF-8 text on the system clipboard.
*/
void UILO::setClipboardText(const std::string& text) {
    if (valo::Window* w = window()) w->setClipboardText(text);
}


/*
    anyKeyDown():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether any key is physically down right now.
*/
bool UILO::anyKeyDown() const {
    valo::Window* w = window();
    if (!w) return false;
    for (size_t k = 0; k < size_t(Key::Count); ++k)
        if (w->isKeyDown(Key(k))) return true;
    return false;
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
    for (auto it = m_overlays.rbegin(); it != m_overlays.rend(); ++it)
        if (it->element->getBounds().contains(pos)) { scrollOverlay = it->element; break; }
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
    for (auto it = m_overlays.rbegin(); it != m_overlays.rend(); ++it)
        if (it->element->getBounds().contains(pos)) { overlay = it->element; break; }
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
    - Nothing here routes events to the window: the monitor consumes every precise
      scroll it sees, and a mouse wheel never reaches it in the first place.
*/
bool UILO::wantsScrollMomentum(const Vec2f& pos) const {
    auto hit = [&](Element* root) -> Element* {
        Element* cur = root;
        while (cur) {
            Container* c = dynamic_cast<Container*>(cur);
            if (!c) return cur;
            /* A floating child owns the pointer outright, so the walk follows it
               and never reaches what it covers. */
            Element* next = c->floatingAt(pos);
            if (!next)
                for (auto* child : c->getChildren()) {
                    if (child->getModifier().isFloating()) continue;
                    if (child->getBounds().contains(pos)) next = child;
                }
            if (!next) return cur;
            cur = next;
        }
        return nullptr;
    };

    Element* root = nullptr;
    for (auto it = m_overlays.rbegin(); it != m_overlays.rend(); ++it)
        if (it->element->getBounds().contains(pos)) { root = it->element; break; }
    if (!root && m_activePage) root = m_activePage->m_rootContainer;
    if (!root) return false;

    Element* deepest = hit(root);
    return deepest ? deepest->wantsScrollMomentum() : true;
}


/*
    pollEvents():
    - Params:   none
    - Returns:  void
    - Desc:     Reads the window's input for the frame and dispatches every event
                in the order it arrived. Closing the window stops the app.
*/
void UILO::pollEvents() {
    valo::Window* w = window();
    if (!w) return;
    if (!w->pollEvents()) m_running = false;
    for (const valo::WindowEvent& event : w->events()) {
        if (event.type == valo::WindowEventType::CloseRequested) m_running = false;
        handleWindowEvent(event);
    }
}

/*
    handleWindowEvent(const valo::WindowEvent& event):
    - Params:   const valo::WindowEvent& event
    - Returns:  void
    - Desc:     Turns one window event into UILO's own dispatch. Scroll and zoom
                gestures are separated here -- a wheel with a modifier held is a
                zoom, not a scroll -- and keyboard events are routed to the
                active interactible, except for the zoom shortcuts and the
                navigation keys, which drive scrolling when nothing has focus. A
                key repeat arriving after every key has physically come up is
                dropped, which is what stops a held key from continuing to fire
                once the window has lost and regained focus. UTF-8 text is
                decoded one codepoint at a time so batched or IME input is not
                dropped.
*/
void UILO::handleWindowEvent(const valo::WindowEvent& event) {
    using Type = valo::WindowEventType;
    if (!m_activePage) return;

    const KeyMod mods  = event.mods;
    const bool   shift = hasMod(mods, KeyMod::Shift);
    const bool   ctrl  = hasMod(mods, KeyMod::Ctrl);
    const bool   gui   = hasMod(mods, KeyMod::Gui);

    if (event.type == Type::Wheel) {
        const float dy = event.wheel.y;
        const float dx = event.wheel.x;
        const bool zoomShortcut = ctrl || gui;
        if (zoomShortcut && dy != 0.f) {
            const float mag = dy * 0.1f;
            dispatchZoom(m_mousePos, mag);
            return;
        }
        if (shift && dx == 0.f && dy != 0.f) {
            const bool precise = std::fabs(dy - std::round(dy)) > 1e-4f
                              || (dy != 0.f && std::fabs(dy) < 1.f);
            dispatchScroll(m_mousePos, Vec2f{dy, 0.f}, precise);
            return;
        }
        const bool precise = std::fabs(dy - std::round(dy)) > 1e-4f
                          || (dy != 0.f && std::fabs(dy) < 1.f);
        dispatchScroll(m_mousePos, Vec2f{dx, dy}, precise);
    }

    /* A press is taken from the event rather than from the polled button state:
       a tap can be pressed and released inside one frame, and by the time
       update() polls, the button reads as up and the press is lost. Its position
       comes from the event too, so it lands where the click was even if the
       pointer has moved on since. */
    if (event.type == Type::MouseDown) {
        if (event.button == MouseButton::Left)  m_pendingLeftPress  = true;
        if (event.button == MouseButton::Right) m_pendingRightPress = true;
        m_pendingPressPos = { event.mouse.x, event.mouse.y };
    }

    if (event.type == Type::KeyUp) {
        m_lastKeyUpNs = event.timestamp;
    }

    if (event.type == Type::Text)
        if (m_currInteractible) {
            if (event.timestamp > m_lastKeyUpNs && !anyKeyDown()) return;

            const unsigned char* s = reinterpret_cast<const unsigned char*>(event.text);
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

    /* An open menu owns the keyboard: it is modal by nature, and letting a
       shortcut through while it is up would act on the UI behind it. */
    if (event.type == Type::KeyDown && isContextMenuOpen()) {
        switch (event.key) {
            case Key::Down:   m_contextMenu->moveHighlight(+1);         return;
            case Key::Up:     m_contextMenu->moveHighlight(-1);         return;
            case Key::Right:  m_contextMenu->deepestOpen()->openHighlightedSubmenu(); return;
            case Key::Left:   if (!m_contextMenu->closeSubmenu()) closeContextMenu(); return;
            case Key::Return:
            case Key::KpEnter: m_contextMenu->activateHighlighted();   return;
            case Key::Escape: if (!m_contextMenu->closeSubmenu()) closeContextMenu(); return;
            default: break;
        }
    }

    if (event.type == Type::KeyDown) {
        const Key key = event.key;
        const bool zoomShortcut = ctrl || gui;
        if (zoomShortcut) {
            if (event.symbol == U'=' || key == Key::KpPlus) {
                dispatchZoom(m_mousePos, 0.1f);
                return;
            }
            if (event.symbol == U'-' || key == Key::KpMinus) {
                dispatchZoom(m_mousePos, -0.1f);
                return;
            }
        }

        if ((key == Key::Up || key == Key::Down ||
             key == Key::PageUp || key == Key::PageDown ||
             key == Key::Home || key == Key::End) &&
            !m_currInteractible) {
            float vertical = 0.f;
            float horizontal = 0.f;
            if (key == Key::Up) vertical = -1.f;
            else if (key == Key::Down) vertical = 1.f;
            else if (key == Key::PageUp) vertical = -3.f;
            else if (key == Key::PageDown) vertical = 3.f;
            else if (key == Key::Home) vertical = -1000.f;
            else if (key == Key::End) vertical = 1000.f;

            if (shift) {
                horizontal = vertical;
                vertical = 0.f;
            }

            if (horizontal != 0.f || vertical != 0.f) {
                dispatchScroll(m_mousePos, Vec2f{horizontal, vertical}, false);
                return;
            }
        }

        if (!m_currInteractible) return;

        if (event.repeat && event.timestamp > m_lastKeyUpNs && !anyKeyDown()) return;

        m_currInteractible->handleKeyInput(key, event.symbol, shift, ctrl, gui);
    }
}

}
