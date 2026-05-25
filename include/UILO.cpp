#include "UILO.hpp"
#include "elements/interactible/Interactible.hpp"
#include <algorithm>

namespace uilo {

UILO::UILO(sf::RenderWindow& window, Page* page) {
    m_window = &window; 
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
        setCurrInteractible(nullptr);
        m_activePage = it->second.get();
    }
}

void UILO::setCurrInteractible(Interactible* i) {
    m_interactibleActivatedThisFrame = true;
    if (m_currInteractible == i) return;
    if (m_currInteractible) m_currInteractible->onDeactivate();
    m_currInteractible = i;
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

void UILO::requestCursor(sf::Cursor::Type type, int priority) {
    if (priority >= m_pendingCursorPriority) {
        m_pendingCursor         = type;
        m_pendingCursorPriority = priority;
    }
}

void UILO::update() {
    m_deltaTime = m_timer.restart();
    m_pendingCursor         = sf::Cursor::Type::Arrow;
    m_pendingCursorPriority = 0;

    if (!m_activePage) return;

    sf::FloatRect logicalBounds = {
        { 0.f, 0.f },
        { static_cast<float>(m_window->getSize().x), static_cast<float>(m_window->getSize().y) }
    };

    m_activePage->update(logicalBounds, m_deltaTime);

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

    sf::Vector2i mouseRaw = sf::Mouse::getPosition(*m_window);
    sf::Vector2f mouse = { static_cast<float>(mouseRaw.x), static_cast<float>(mouseRaw.y) };

    bool leftDown  = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    bool rightDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

    auto* root = m_activePage->m_rootContainer;

    // Hover: Resizers first (they render on top), then overlays
    for (auto* r : m_resizers) r->checkHover(mouse);
    Element* hoveredOverlay = nullptr;
    for (auto& ov : m_overlays)
        if (ov.element->getBounds().contains(mouse)) { hoveredOverlay = ov.element; break; }
    if (hoveredOverlay) hoveredOverlay->checkHover(mouse);
    else                root->checkHover(mouse);

    // Apply pending cursor (lazy-init, only swap when type changes)
    if (m_pendingCursor != m_activeCursor) {
        const sf::Cursor* cur = nullptr;
        switch (m_pendingCursor) {
            case sf::Cursor::Type::Arrow:
                if (!m_curArrow) m_curArrow = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);
                if (m_curArrow) cur = &*m_curArrow;
                break;
            case sf::Cursor::Type::Hand:
                if (!m_curHand) m_curHand = sf::Cursor::createFromSystem(sf::Cursor::Type::Hand);
                if (m_curHand) cur = &*m_curHand;
                break;
            case sf::Cursor::Type::SizeHorizontal:
                if (!m_curSizeH) m_curSizeH = sf::Cursor::createFromSystem(sf::Cursor::Type::SizeHorizontal);
                if (m_curSizeH) cur = &*m_curSizeH;
                break;
            case sf::Cursor::Type::SizeVertical:
                if (!m_curSizeV) m_curSizeV = sf::Cursor::createFromSystem(sf::Cursor::Type::SizeVertical);
                if (m_curSizeV) cur = &*m_curSizeV;
                break;
            default: break;
        }
        if (cur && m_window) m_window->setMouseCursor(*cur);
        m_activeCursor = m_pendingCursor;
    }

    if (leftDown && !m_prevLeftMouse) {
        m_interactibleActivatedThisFrame = false;

        // Resizers get click priority (they render on top)
        bool resizerClicked = false;
        for (auto* r : m_resizers) {
            if (r->getBounds().contains(mouse)) {
                r->checkLeftClick(mouse);
                resizerClicked = true;
                break;
            }
        }

        if (!resizerClicked) {
            // Find which overlay (if any) was clicked
            Element* clickedOverlay = nullptr;
            for (auto& ov : m_overlays)
                if (ov.element->getBounds().contains(mouse)) { clickedOverlay = ov.element; break; }

            if (clickedOverlay) {
                clickedOverlay->checkLeftClick(mouse);
            } else {
                // Outside all overlays — dismiss them, then pass click to page
                auto copy = m_overlays;
                m_overlays.clear();
                for (auto& ov : copy) if (ov.onDismiss) ov.onDismiss();
                root->checkLeftClick(mouse);
                if (!m_interactibleActivatedThisFrame) setCurrInteractible(nullptr);
            }
        }
    }
    if (rightDown && !m_prevRightMouse) {
        m_interactibleActivatedThisFrame = false;
        if (!m_overlays.empty()) {
            auto copy = m_overlays;
            m_overlays.clear();
            for (auto& ov : copy) if (ov.onDismiss) ov.onDismiss();
        }
        root->checkRightClick(mouse);
        if (!m_interactibleActivatedThisFrame) setCurrInteractible(nullptr);
    }

    m_prevLeftMouse  = leftDown;
    m_prevRightMouse = rightDown;
}

void UILO::render() {
    // Set a pixel-accurate view based on the current window size before rendering.
    // Without this, a resized window would leave the view based on the original
    // creation size (getDefaultView()), causing overlays and resizers to render
    // at the wrong screen position even though their m_bounds are correct.
    const auto ws = m_window->getSize();
    m_window->setView(sf::View(sf::FloatRect{
        {0.f, 0.f},
        {static_cast<float>(ws.x), static_cast<float>(ws.y)}
    }));

    m_activePage->render(*m_window);
    for (auto& ov : m_overlays)
        ov.element->render(*m_window);
    for (auto* r : m_resizers)
        r->render(*m_window);
}

void UILO::handleEvent(const sf::Event& event) {
    if (!m_activePage) return;

    if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        sf::Vector2f mouse = {
            static_cast<float>(scroll->position.x),
            static_cast<float>(scroll->position.y)
        };
        Element* scrollOverlay = nullptr;
        for (auto& ov : m_overlays)
            if (ov.element->getBounds().contains(mouse)) { scrollOverlay = ov.element; break; }
        if (scrollOverlay) scrollOverlay->checkScroll(mouse, scroll->delta);
        else               m_activePage->m_rootContainer->checkScroll(mouse, scroll->delta);
    }
}

}