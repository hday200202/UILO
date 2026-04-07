#include "Page.hpp"
#include "UILO.hpp"

namespace uilo {

Page::Page(Container* rootContainer, const std::string& name) {
    m_rootContainer = rootContainer;
    m_name = name;
}

void Page::update(Bounds& screenBounds, float dt) {
    m_rootContainer->update(screenBounds, dt);
}

void Page::render(Renderer& renderer) {
    m_rootContainer->render(renderer);
}

void Page::setUilo(UILO& uiloRef) {
    m_uiloRef = &uiloRef;
    m_rootContainer->setUilo(uiloRef);
}

} // namespace uilo