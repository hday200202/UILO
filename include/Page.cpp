#include "Page.hpp"
#include "UILO.hpp"

namespace uilo {

/*
    Page(Container* rootContainer, const std::string& name):
    - Params:   Container* rootContainer, const std::string& name
    - Returns:  Page
    - Desc:     Constructs a page from a root container and a name.
*/
Page::Page(Container* rootContainer, const std::string& name) {
    m_rootContainer = rootContainer;
    m_name = name;
}


/*
    update(Rectf& screenBounds, float dt):
    - Params:   Rectf& screenBounds, float dt
    - Returns:  void
    - Desc:     Lays out the page by ticking its root container against the
                given screen bounds and delta time.
*/
void Page::update(Rectf& screenBounds, float dt) {
    m_rootContainer->tick(screenBounds, dt);
}


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the page by rendering its root container.
*/
void Page::render() {
    m_rootContainer->render();
}


/*
    setUILO(UILO& uiloRef):
    - Params:   UILO& uiloRef
    - Returns:  void
    - Desc:     Binds the page and its root container to a UILO instance.
*/
void Page::setUILO(UILO& uiloRef) {
    m_uiloRef = &uiloRef;
    m_rootContainer->setUILO(uiloRef);
}

}
