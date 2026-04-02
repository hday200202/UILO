#pragma once

#include <string>

#include "Modifier.hpp"
#include "Bounds.hpp"
#include "../graphics/Renderer.hpp"

namespace uilo {

class UILO;

enum class ElementType {
    // Used for base classes of elements
        NONE,

    // Container
        Column,
        Row,
        ScrollableColumn,
        ScrollableRow,
        FreeColumn,
        FreeRow,
        Grid,
    
    // Decor
        Spacer,
        Text,
        Image,

    // Interactible
        Button,
        Slider,
        Textbox,
};

class Element {
public:
    Element()                   = default;
    virtual ~Element()          = default;

    Bounds getBounds() const;   // stores position and size
    Modifier& getModifier();    // stores color, alignment, visibility, ...
    bool isDirty() const;       // should this element be updated
    void erase();
    virtual void setUilo(UILO& uiloRef);

    virtual void update(Bounds& parentBounds, float dt) = 0;
    virtual void render(Renderer& renderer) = 0;
    virtual bool checkRightClick(const Vec2f& mousePosition);
    virtual bool checkLeftClick(const Vec2f& mousePosition);
    virtual bool checkHover(const Vec2f& mousePosition);
    virtual bool checkScroll(const Vec2f& mousePosition, float delta);
    void resize(const Bounds& parent);

    ElementType getType() const;

protected:
    UILO* m_uiloRef             = nullptr;
    std::string m_name          = "";

    Bounds m_bounds             = {{0, 0}, {0, 0}};
    Bounds m_pastBounds         = {{0, 0}, {0, 0}};

    bool m_dirty                = true;
    bool m_markedForDeletion    = false;
    bool m_hovered              = false;
    
    ElementType m_type          = ElementType::NONE;
    Modifier m_modifier         = Modifier();

    friend class UILO;
    friend class Container;
};

}