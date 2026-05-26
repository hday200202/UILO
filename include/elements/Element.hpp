#pragma once

#include <string>
#include <vector>

#include "Modifier.hpp"
#include "../../include/utils/Math.hpp"

namespace uilo {

/*
    All element types finished or planned
*/
enum class ElementType {
    NONE,

    Column,
    Row,
    ScrollableColumn,
    ScrollableRow,
    Grid,

    Spacer,
    Text,
    Image,

    Button,
    Slider,
    Dropdown,
    Knob,
    TextBox,
    Resizer,
};

class UILO;

class Element {
public:
    Element() = default;
    virtual ~Element() = default;

    Rectf getBounds() const;
    Modifier& getModifier();
    virtual bool isDirty() const;
    void erase();

    virtual void setUILO(UILO& uiloRef);
    virtual void update(Rectf& parentBounds, float dt) = 0;
    virtual void render() = 0;
    virtual bool checkLeftClick(const Vec2f& mousePosition);
    virtual bool checkRightClick(const Vec2f& mousePosition);
    virtual bool checkHover(const Vec2f& mousePosition);
    virtual bool checkScroll(const Vec2f& mousePosition, float delta);
    void resize(const Rectf& parent);

    // Recursively collect all Resizer-type descendant elements
    virtual void collectResizers(std::vector<Element*>&) {}

    ElementType getType() const;

protected:
    UILO* m_uiloRef             = nullptr;
    std::string m_name          = "";

    Rectf m_bounds     = {};
    Rectf m_pastBounds = {};

    bool m_dirty                = true;
    bool m_markedForDeletion    = false;
    bool m_hovered              = false;

    ElementType m_type          = ElementType::NONE;
    Modifier m_modifier         = Modifier();

    friend class UILO;
    friend class Container;
};

}