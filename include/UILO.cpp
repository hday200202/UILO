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

void UILO::setOnResize(std::function<void(float, float)> callback) {
    m_onResize = std::move(callback);
}

void UILO::update(const Input& input) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastTime).count();
    m_lastTime = now;

    if (!m_activePage) return;

    m_activePage->update(m_screenBounds, dt);

    auto* root = m_activePage->m_rootContainer;
    root->checkHover(input.mousePosition);
    if (input.leftMouse)          root->checkLeftClick(input.mousePosition);
    if (input.rightMouse)         root->checkRightClick(input.mousePosition);
    if (input.scrollDelta != 0.f) root->checkScroll(input.mousePosition, input.scrollDelta);
}

void UILO::render(Renderer& renderer) {
    if (!m_activePage) return;
    m_activePage->render(renderer);
}

} // namespace uilo
