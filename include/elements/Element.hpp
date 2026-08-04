#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Modifier.hpp"
#include "../../include/utils/Math.hpp"
#include "../utils/Gradient.hpp"

namespace uilo {

/*
    ElementType:
    - Desc:     Which kind of element an instance is, for the cases where
                behaviour has to branch on it rather than on a virtual call:
                layout skipping Resizers, hit-testing looking for Interactibles,
                and the Wt bridge choosing which widget to build. Includes types
                that are planned but not yet implemented.
*/
enum class ElementType {
    NONE,

    Column,
    Row,
    ScrollableColumn,
    ScrollableRow,
    Grid,
    Canvas,

    Spacer,
    Text,
    Image,
    Icon,
    Waveform,

    Button,
    Slider,
    Dropdown,
    Knob,
    TextBox,
    Resizer,
    Terminal,
};

class UILO;

/*
    Element:
    - Desc:     Base class for everything UILO draws. An element owns its
                Modifier (size, alignment, padding, callbacks), its resolved
                bounds, and the flags that drive redrawing and deletion.
                Subclasses supply update() and render(); the pointer-event
                checks have working defaults that read the Modifier's callbacks.
                Ownership belongs to UILO rather than to the parent: setUILO()
                puts the element in the element pool, and erase() only marks it,
                so a handler is free to remove an element while the tree is
                still being walked. Colors and gradients resolve through the
                owning UILO's Palette, which is why an unbound element falls
                back to its literal values.
*/
class Element {
public:
    Element() = default;
    virtual ~Element() = default;

    Rectf        getBounds() const;
    Modifier&    getModifier();
    virtual bool isDirty() const;
    bool         isHovered() const { return m_hovered; }
    UILO*        getUILO()   const { return m_uiloRef; }
    bool         takesPointerEvents() const;
    float        getDeltaTime() const;

    Color resolveColor(std::string_view role, Color literal) const;
    bool  resolveGradient(
        const Gradient& literal,
        std::string_view gradientRole,
        Color out[4]
    ) const;
    void  erase();

    virtual void setUILO(UILO& uiloRef);
    virtual void update(Rectf& parentBounds, float dt) = 0;
    void         tick(Rectf& parentBounds, float dt);
    virtual void render() = 0;

    virtual bool checkLeftClick(const Vec2f& mousePosition);
    virtual bool checkRightClick(const Vec2f& mousePosition);
    virtual bool checkHover(const Vec2f& mousePosition);
    virtual bool checkScroll(
        const Vec2f& mousePosition,
        float delta,
        bool precise = false,
        bool momentum = false
    );
    virtual bool checkScroll(
        const Vec2f& mousePosition,
        Vec2f delta,
        bool precise = false,
        bool momentum = false
    );
    virtual bool checkZoom(const Vec2f& mousePosition, float magnification);

    // Whether a trackpad flick that ends over this element should coast. True
    // for anything that scrolls a view; false for widgets where a scroll
    // changes a value, since a coast there would keep cranking the value after
    // the fingers have left.
    virtual bool wantsScrollMomentum() const { return true; }

    // Padding lives in each widget's Options rather than in Modifier: not every
    // element can hold content, so having the outer one on Modifier and the
    // inner one on Options would split a single idea across two objects. Layout
    // is generic, so it asks through these and each widget answers from its own
    // Options. Zero for an element whose Options does not offer one.
    virtual float getOuterPadding() const { return 0.f; }
    virtual float getInnerPadding() const { return 0.f; }

    // The element's bounds inset by its inner padding: where its content goes.
    // Never inverted -- padding wider than the box collapses the area to zero
    // rather than turning it inside out.
    Rectf contentArea() const;

    void resize(const Rectf& parent);

    virtual void collectResizers(std::vector<Element*>&) {}

    ElementType        getType() const;
    const std::string& getName() const { return m_name; }

protected:
    virtual bool claimsPointerEvents() const;

    UILO*       m_uiloRef = nullptr;
    std::string m_name    = "";

    Rectf m_bounds     = {};
    Rectf m_pastBounds = {};

    bool m_dirty             = true;
    bool m_markedForDeletion = false;
    bool m_hovered           = false;

    ElementType m_type     = ElementType::NONE;
    Modifier    m_modifier = Modifier();

    friend class UILO;
    friend class Container;
    friend class Canvas;
};


/*
    takesPointerEvents():
    - Params:   none
    - Returns:  bool
    - Desc:     Public read of claimsPointerEvents(), for the web bridge. Native
                hit-testing stops at the first element that claims a click, so
                ancestors never see it, but a DOM click bubbles to every
                ancestor's handler. The bridge stops propagation on exactly the
                elements that claim, which is what keeps a click on a popup's
                arrow from also reaching the scrim behind it and dismissing it.
*/
inline bool Element::takesPointerEvents() const {
    return claimsPointerEvents();
}


/*
    checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, Vec2f delta, bool precise, bool
                momentum
    - Returns:  bool -- true when the event was consumed
    - Desc:     Two-axis scroll. Forwards delta.y to the single-axis overload,
                so every widget is vertical-only without having to say so.
                Elements that consume both axes -- Canvas, Row, Column --
                override it.
*/
inline bool Element::checkScroll(
    const Vec2f& mousePosition,
    Vec2f delta,
    bool precise,
    bool momentum
) {
    return checkScroll(mousePosition, delta.y, precise, momentum);
}


/*
    checkZoom(const Vec2f& mousePosition, float magnification):
    - Params:   const Vec2f& mousePosition, float magnification
    - Returns:  bool -- true when the gesture was consumed
    - Desc:     Pinch or programmatic zoom. `magnification` is an additive per-
                event ratio, so 0.05 means grow by five percent. Declines by
                default; Canvas and the containers override to consume or
                forward it.
*/
inline bool Element::checkZoom(const Vec2f& mousePosition, float magnification) {
    (void)mousePosition;
    (void)magnification;
    return false;
}

}
