#pragma once

#include <vector>
#include <initializer_list>
#include "../Element.hpp"

namespace uilo {

using contains = std::initializer_list<Element*>;

class Container : public Element{
public:
    Container(Modifier modifier, contains children, const std::string& name);

    void update(sf::FloatRect& parentBounds, float dt) override = 0;
    void render(sf::RenderTarget& target) override = 0;

    bool checkRightClick(const sf::Vector2f& mousePosition) override;
    bool checkLeftClick(const sf::Vector2f& mousePosition) override;
    bool checkHover(const sf::Vector2f& mousePosition) override;
    bool checkScroll(const sf::Vector2f& mousePosition, float delta) override;

    void addElement(Element* element);
    void setUILO(UILO& uiloRef) override;

protected:
    std::vector<Element*> m_children;
    sf::RenderTexture m_rt;
    void pruneChildren();
};

}