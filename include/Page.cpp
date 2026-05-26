#include "Page.hpp"
#include "UILO.hpp"

namespace uilo {

Page::Page(Container* rootContainer, const std::string& name) {
    m_rootContainer = rootContainer;
    m_name = name;
}

void Page::update(Rectf& screenBounds, float dt) {
    m_rootContainer->update(screenBounds, dt);
}

void Page::render() {
    m_rootContainer->render();
}

void Page::setUILO(UILO& uiloRef) {
    m_uiloRef = &uiloRef;
    m_rootContainer->setUILO(uiloRef);
}

}