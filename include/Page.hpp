#pragma once

#include "elements/containers/Container.hpp"

namespace uilo {

class UILO;

/*
    Page:
    - Desc: A named screen owned by UILO, wrapping a root Container. UILO
            ticks and renders the active page each frame, and the page
            forwards layout and draw calls to its root container. Members
            are protected and driven through UILO via friendship.
*/
class Page {
public:
    Page(Container* rootContainer, const std::string& name);

protected:
    UILO* m_uiloRef = nullptr;
    Container* m_rootContainer = nullptr;
    std::string m_name = "";

    void update(Rectf& screenBounds, float dt);
    void render();
    void setUILO(UILO& uiloRef);

    friend class UILO;
};

}