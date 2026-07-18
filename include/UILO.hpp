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

    void handleEvent(const SDL_Event& event);
    void dispatchScroll(const Vec2f& pos, Vec2f delta, bool precise, bool momentum = false);
    void dispatchZoom(const Vec2f& pos, float magnification);
    bool isSDLScrollTarget(const Vec2f& pos) const;

    void setRenderer(Renderer& renderer) { m_renderer = &renderer; }
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
    
    float getScale()                const { return m_scale; }
    float getDeltaTime()            const { return m_deltaTime; }
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