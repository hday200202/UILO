#pragma once

#include <string>
#include <string_view>
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
    Canvas,

    Spacer,
    Text,
    Image,
    Waveform,

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
    bool isHovered() const { return m_hovered; }
    UILO* getUILO() const { return m_uiloRef; }
    float getDeltaTime() const; // defined in Element.cpp (needs UILO complete type)
    // Resolves a per-widget color through the owning UILO's Palette.
    // Returns `literal` unchanged when there's no UILO or the role is
    // empty/"none" or unknown. Defined in Element.cpp.
    Color resolveColor(std::string_view role, Color literal) const;
    void erase();

    virtual void setUILO(UILO& uiloRef);
    virtual void update(Rectf& parentBounds, float dt) = 0;
    // Non-virtual wrapper that fires onUpdateStart/onUpdateEnd around the
    // virtual update(). Callers in the tree (containers, page, dropdown
    // popup, etc.) should invoke tick() rather than update() directly so
    // the lifecycle hooks always fire.
    void tick(Rectf& parentBounds, float dt) {
        if (m_modifier.getOnUpdateStart()) m_modifier.getOnUpdateStart()(this);
        update(parentBounds, dt);
        if (m_modifier.getOnUpdateEnd())   m_modifier.getOnUpdateEnd()(this);
    }
    virtual void render() = 0;
    virtual bool checkLeftClick(const Vec2f& mousePosition);
    virtual bool checkRightClick(const Vec2f& mousePosition);
    virtual bool checkHover(const Vec2f& mousePosition);
    virtual bool checkScroll(const Vec2f& mousePosition, float delta, bool precise = false, bool momentum = false);
    // 2-axis scroll. Default forwards to the 1D overload using delta.y so
    // every existing widget stays Y-only without modification; containers
    // and widgets that consume both axes (Canvas, etc.) override this.
    virtual bool checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise = false, bool momentum = false) {
        return checkScroll(mousePosition, delta.y, precise, momentum);
    }
    // Pinch / programmatic zoom dispatch. `magnification` is a per-event
    // additive ratio (e.g. 0.05 = grow 5%). Default returns false; Canvas
    // and containers override to consume / forward.
    virtual bool checkZoom(const Vec2f& mousePosition, float magnification) {
        (void)mousePosition; (void)magnification; return false;
    }
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
    friend class Canvas;
};

}