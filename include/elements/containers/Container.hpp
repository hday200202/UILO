#pragma once

#include <vector>
#include <initializer_list>
#include "../Element.hpp"
#include "../renderer/Renderer.hpp"

namespace uilo {

using contains = std::initializer_list<Element*>;

class Container : public Element{
public:
    Container(Modifier modifier, contains children, const std::string& name);

    void update(Rectf& parentBounds, float dt) override = 0;
    void render() override = 0;

    bool checkRightClick(const Vec2f& mousePosition) override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkHover(const Vec2f& mousePosition) override;
    bool checkScroll(const Vec2f& mousePosition, float delta) override;

    void addElement(Element* element);
    void setUILO(UILO& uiloRef) override;
    void collectResizers(std::vector<Element*>& out) override;
    bool isDirty() const override;

protected:
    std::vector<Element*> m_children;
    FrameBuffer m_fb;  // per-container render target (replaces sf::RenderTexture m_rt)
    void pruneChildren();
};

}