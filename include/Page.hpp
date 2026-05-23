#pragma once

#include "elements/containers/Container.hpp"

namespace uilo {

class UILO;

class Page {
public:
    Page(Container* rootContainer, const std::string& name);

protected:
    UILO* m_uiloRef = nullptr;
    Container* m_rootContainer = nullptr;
    std::string m_name = "";

    void update(sf::FloatRect& screenBounds, float dt);
    void render(sf::RenderTarget& target);
    void setUILO(UILO& uiloRef);

    friend class UILO;
};

}