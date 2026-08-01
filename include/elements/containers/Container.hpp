#pragma once

#include <vector>
#include <initializer_list>
#include "../Element.hpp"
#include "../renderer/Renderer.hpp"

namespace uilo {

using contains = std::initializer_list<Element*>;

/*
    Container:
    - Desc: Base class for every element that holds others. It owns the child
            list and the pointer-event plumbing -- clicks, hover, scroll and
            zoom are offered to the children before the container itself gets a
            look -- and leaves layout and drawing abstract for Row, Column and
            Canvas to define. Children are raw pointers, since UILO's element
            pool owns them; pruneChildren() drops any that have been erased, and
            m_fb is the container's own render target.
*/
class Container : public Element {
public:
    Container(Modifier modifier, contains children, const std::string& name);

    void update(Rectf& parentBounds, float dt) override = 0;
    void render() override = 0;

    bool checkRightClick(const Vec2f& mousePosition) override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkHover(const Vec2f& mousePosition) override;
    bool checkScroll(
        const Vec2f& mousePosition,
        float delta,
        bool precise = false,
        bool momentum = false
    ) override;
    bool checkScroll(
        const Vec2f& mousePosition,
        Vec2f delta,
        bool precise = false,
        bool momentum = false
    ) override;
    bool checkZoom(const Vec2f& mousePosition, float magnification) override;

    void addElement(Element* element);
    void setUILO(UILO& uiloRef) override;
    void collectResizers(std::vector<Element*>& out) override;
    bool isDirty() const override;

    const std::vector<Element*>& getChildren() const { return m_children; }

protected:
    std::vector<Element*> m_children;
    FrameBuffer           m_fb;

    void pruneChildren();
};

}
