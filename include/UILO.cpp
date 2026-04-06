#include "UILO.hpp"

namespace uilo {

UILO::UILO()
    : m_lastTime(std::chrono::steady_clock::now())
{}

void UILO::addPage(Page* page) {
    page->setUilo(*this);
    m_pages[page->m_name] = std::unique_ptr<Page>(page);
}

void UILO::setPage(const std::string& pageName) {
    auto it = m_pages.find(pageName);
    if (it != m_pages.end())
        m_activePage = it->second.get();
}

void UILO::setScreenBounds(const Bounds& bounds) {
    m_screenBounds = bounds;
    if (m_onResize)
        m_onResize(bounds.size.x, bounds.size.y);
}

void UILO::setScale(float scale) {
    if (scale > 0.f) m_scale = scale;
}

void UILO::setOnResize(std::function<void(float, float)> callback) {
    m_onResize = std::move(callback);
}

void UILO::update(const Input& input) {
    auto now = std::chrono::steady_clock::now();
    m_deltaTime = std::chrono::duration<float>(now - m_lastTime).count();
    m_lastTime = now;

    if (!m_activePage) return;

    Bounds logicalBounds = {{m_screenBounds.position.x / m_scale, m_screenBounds.position.y / m_scale},
                             {m_screenBounds.size.x     / m_scale, m_screenBounds.size.y     / m_scale}};
    m_activePage->update(logicalBounds, m_deltaTime);

    // Free elements marked for deletion
    m_elementPool.erase(
        std::remove_if(m_elementPool.begin(), m_elementPool.end(),
            [&](const std::unique_ptr<Element>& e) {
                if (e->m_markedForDeletion) {
                    if (!e->m_name.empty())
                        m_elements.erase(e->m_name);
                    return true;
                }
                return false;
            }),
        m_elementPool.end());

    // Divide mouse by scale so hit testing works in layout coords
    Vec2f mouse = {input.mousePosition.x / m_scale, input.mousePosition.y / m_scale};
    auto* root = m_activePage->m_rootContainer;
    root->checkHover(mouse);
    if (input.leftMouse)          root->checkLeftClick(mouse);
    if (input.rightMouse)         root->checkRightClick(mouse);
    if (input.scrollDelta != 0.f) root->checkScroll(mouse, input.scrollDelta);
}

void UILO::render(Renderer& renderer) {
    if (!m_activePage) return;
    renderer.m_renderScale = m_scale;
    renderer.beginFrame();
    m_activePage->render(renderer);
}

} // namespace uilo
