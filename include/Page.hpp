#pragma once

#include "elements/Elements.hpp"

namespace uilo {

class UILO;

class Page {
public:
    Page(Container* rootContainer, const std::string& name);

protected:
    UILO* m_uiloRef = nullptr;
    Container* m_rootContainer = nullptr;
    std::string m_name = "";

    void update(Bounds& screenBounds, float dt);
    void render(Renderer& renderer);
    void setUilo(UILO& uiloRef);

    friend class UILO;
};

}