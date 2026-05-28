#include "UILO.hpp"
#include "elements/interactible/Interactible.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

namespace uilo {

UILO::UILO(Renderer& renderer, Page* page) {
    m_renderer = &renderer;
    addPage(page);
    setPage(page->m_name);
}

void UILO::addPage(Page* page) {
    page->setUILO(*this);
    m_pages[page->m_name] = std::unique_ptr<Page>(page);
}

void UILO::setPage(const std::string& pageName) {
    auto it = m_pages.find(pageName);
    if (it != m_pages.end()) {
        m_overlays.clear();
        m_resizers.clear();
        m_floating.clear();
        setCurrInteractible(nullptr);
        m_activePage = it->second.get();
    }
}

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

void UILO::setScale(float scale) { if (scale > 0.f) m_scale = scale; }

void UILO::registerOverlay(Element* e, std::function<void()> onDismiss) {
    for (auto& ov : m_overlays)
        if (ov.element == e) return;  // already registered
    m_overlays.push_back({e, std::move(onDismiss)});
}

void UILO::unregisterOverlay(Element* e) {
    m_overlays.erase(
        std::remove_if(m_overlays.begin(), m_overlays.end(),
            [e](const OverlayEntry& ov) { return ov.element == e; }),
        m_overlays.end()
    );
}

Element* UILO::addFloating(FreeElement f) {
    if (!f.element) return nullptr;
    f.element->setUILO(*this);  // registers in m_elementPool + m_elements (recursively for containers)
    FloatingEntry entry;
    entry.element   = f.element;
    entry.xPos      = f.xPos;
    entry.yPos      = f.yPos;
    entry.draggable = f.draggable;
    m_floating.push_back(entry);
    return f.element;
}

void UILO::removeFloating(Element* e) {
    m_floating.erase(
        std::remove_if(m_floating.begin(), m_floating.end(),
            [e](const FloatingEntry& f) { return f.element == e; }),
        m_floating.end()
    );
}

void UILO::requestCursor(CursorType type, int priority) {
    if (priority >= m_pendingCursorPriority) {
        m_pendingCursor         = type;
        m_pendingCursorPriority = priority;
    }
}

void UILO::update() {
    m_deltaTime = m_timer.restart();
    m_pendingCursor         = CursorType::Arrow;
    m_pendingCursorPriority = 0;

    if (!m_activePage) return;

    const Vec2u windowSize = m_renderer->getSize();
    if (windowSize != m_prevWindowSize) {
        for (auto& e : m_elementPool) e->m_dirty = true;
        m_prevWindowSize = windowSize;
    }

    Rectf logicalBounds = {
        { 0.f, 0.f },
        { static_cast<float>(windowSize.x), static_cast<float>(windowSize.y) }
    };

    m_activePage->update(logicalBounds, m_deltaTime);

    const float winW  = static_cast<float>(windowSize.x);
    const float winH  = static_cast<float>(windowSize.y);
    const float scale = m_scale;
    for (auto& f : m_floating) {
        // Scale only applies to fixed-pixel positions — percent positions
        // resolve against the full window so they stay anchored.
        const float x = f.xPos.percent ? (f.xPos.value / 100.f * winW)
                                       : (f.xPos.value * scale);
        const float y = f.yPos.percent ? (f.yPos.value / 100.f * winH)
                                       : (f.yPos.value * scale);
        // Slot size = full window so percent widths/heights on the
        // element resolve against window size. The element will compute
        // its own real bounds inside tick() via its Modifier.
        Rectf slot = { {x, y}, {winW, winH} };
        f.element->tick(slot, m_deltaTime);
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

    float mx, my;
    SDL_GetMouseState(&mx, &my);
    // SDL reports mouse in logical (point) coordinates, but our layout is
    // sized in backing pixels. Convert by the window's pixel density.
    if (SDL_Window* w = m_renderer->sdlWindow()) {
        int lw = 1, lh = 1, pw = 1, ph = 1;
        SDL_GetWindowSize(w, &lw, &lh);
        SDL_GetWindowSizeInPixels(w, &pw, &ph);
        if (lw > 0) mx *= (float)pw / (float)lw;
        if (lh > 0) my *= (float)ph / (float)lh;
    }
    m_mousePos = { mx, my };
    const Vec2f mouse = m_mousePos;

    // Feed the cursor through to the renderer so interactive Materials
    // (Ripple / Hover) can sample it without each element needing to.
    if (m_renderer) m_renderer->setMouseState(mouse);

    bool leftDown  = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))  != 0;
    bool rightDown = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;

    auto* root = m_activePage->m_rootContainer;

    // Topmost floating element under the cursor (if any). Floating
    // elements are opaque to input: they fully consume hover, cursor
    // requests, and clicks for the region they cover — the page beneath
    // never sees the event. Iterate back-to-front because later entries
    // render on top.
    FloatingEntry* hoveredFloating = nullptr;
    for (auto it = m_floating.rbegin(); it != m_floating.rend(); ++it) {
        if (!it->element->getModifier().getVisible()) continue;
        if (it->element->getBounds().contains(mouse)) {
            hoveredFloating = &(*it);
            break;
        }
    }

    // Floating draggable HUDs: start drag on press-in-bounds, follow the
    // mouse while held. Any click on a floating element is consumed.
    bool floatingConsumedClick = false;
    if (leftDown) {
        if (!m_prevLeftMouse) {
            if (hoveredFloating) {
                floatingConsumedClick = true;
                if (hoveredFloating->draggable) {
                    const Rectf& vb = hoveredFloating->element->getBounds();
                    hoveredFloating->dragging   = true;
                    hoveredFloating->dragOffset = { mouse.x - vb.position.x,
                                                    mouse.y - vb.position.y };
                }
            }
        }
        for (auto& f : m_floating) {
            if (f.dragging) {
                // While dragging, snap position to absolute (unscaled)
                // pixels so the cursor tracks 1:1 regardless of UI scale.
                const float sc = m_scale > 0.f ? m_scale : 1.f;
                f.xPos = { (mouse.x - f.dragOffset.x) / sc, false };
                f.yPos = { (mouse.y - f.dragOffset.y) / sc, false };
                requestCursor(CursorType::Crosshair, 10);
            }
        }
    } else {
        for (auto& f : m_floating) f.dragging = false;
        if (hoveredFloating && hoveredFloating->draggable)
            requestCursor(CursorType::Crosshair, 5);
    }

    // Hover: Resizers first (they render on top), then overlays, then
    // either the topmost floating (if hovered) or the page root. The
    // floating branch is exclusive — the page does NOT also receive
    // hover, so cursor/state requests from elements behind the floating
    // panel can't leak through.
    for (auto* r : m_resizers) r->checkHover(mouse);
    Element* hoveredOverlay = nullptr;
    for (auto& ov : m_overlays)
        if (ov.element->getBounds().contains(mouse)) { hoveredOverlay = ov.element; break; }
    if (hoveredOverlay)      hoveredOverlay->checkHover(mouse);
    else if (hoveredFloating) hoveredFloating->element->checkHover(mouse);
    else                     root->checkHover(mouse);

    // Apply pending cursor
    if (m_pendingCursor != m_activeCursor) {
        m_renderer->setCursor(m_pendingCursor);
        m_activeCursor = m_pendingCursor;
    }

    if (leftDown && !m_prevLeftMouse && !floatingConsumedClick) {
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
    } else if (leftDown && !m_prevLeftMouse && hoveredFloating) {
        // Dispatch the click to the floating element so interactives
        // inside it still respond. The page beneath remains shielded.
        m_interactibleActivatedThisFrame = false;
        hoveredFloating->element->checkLeftClick(mouse);
    }
    if (rightDown && !m_prevRightMouse) {
        m_interactibleActivatedThisFrame = false;
        if (!m_overlays.empty()) {
            auto copy = m_overlays;
            m_overlays.clear();
            for (auto& ov : copy) if (ov.onDismiss) ov.onDismiss();
        }
        if (hoveredFloating) hoveredFloating->element->checkRightClick(mouse);
        else                 root->checkRightClick(mouse);
        if (!m_interactibleActivatedThisFrame) setCurrInteractible(nullptr);
    }

    m_prevLeftMouse  = leftDown;
    m_prevRightMouse = rightDown;
}

void UILO::render() {
    if (!m_activePage) return;

    m_activePage->render();
    for (auto& f : m_floating)
        f.element->render();
    for (auto& ov : m_overlays)
        ov.element->render();
    for (auto* r : m_resizers)
        r->render();
}

void UILO::handleEvent(const SDL_Event& event) {
    if (!m_activePage) return;

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        const Vec2f mouse = m_mousePos;
        float delta = event.wheel.y;  // positive = scroll up
        Element* scrollOverlay = nullptr;
        for (auto& ov : m_overlays)
            if (ov.element->getBounds().contains(mouse)) { scrollOverlay = ov.element; break; }
        if (scrollOverlay) scrollOverlay->checkScroll(mouse, delta);
        else               m_activePage->m_rootContainer->checkScroll(mouse, delta);
    }

    if (event.type == SDL_EVENT_KEY_UP) {
        m_lastKeyUpNs = event.common.timestamp;
    }

    if (event.type == SDL_EVENT_TEXT_INPUT)
        if (m_currInteractible) {
            // Wayland tends to deliver key-repeat events (and their
            // matching TEXT_INPUTs) a moment after the user has physically
            // released the key. If this TEXT_INPUT was generated AFTER
            // the most recent KEY_UP and no key is currently held, treat
            // it as a stale repeat and drop it.
            if (event.common.timestamp > m_lastKeyUpNs) {
                int nKeys = 0;
                const bool* state = SDL_GetKeyboardState(&nKeys);
                bool anyDown = false;
                for (int i = 0; i < nKeys; ++i) {
                    if (state[i]) { anyDown = true; break; }
                }
                if (!anyDown) return;
            }

            // SDL_EVENT_TEXT_INPUT gives a UTF-8 string that may contain
            // multiple codepoints (IME / batched repeats on Wayland).
            // Dispatch every codepoint so we don't drop characters.
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

    if (event.type == SDL_EVENT_KEY_DOWN)
        if (m_currInteractible) {
            // Same stale-repeat filter for key-only events (backspace,
            // arrows, etc. don't produce TEXT_INPUT).
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
            m_currInteractible->handleKeyInput(event.key.key, shift, ctrl);
        }
}

}