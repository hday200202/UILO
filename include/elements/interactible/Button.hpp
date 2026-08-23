#pragma once

#include <optional>

#include "../containers/Row.hpp"
#include "../decoration/Text.hpp"
#include "../../utils/Theme.hpp"

#include "../../utils/Themed.hpp"

namespace uilo {

/*
    ButtonOptions:
    - Desc:     Everything a Button draws: background fill or gradient, corner
                rounding, an inside border, and the Text element used as its
                label. Colors come as a literal plus a role, where the role wins
                when it resolves against the active Palette and the literal is
                the fallback. These are a curated subset of RowOptions, since a
                Button is a Row -- scrolling and the subdivision grid are
                deliberately not exposed.
*/
class ButtonOptions {
public:
    UILO_THEMED(ButtonOptions)

    /*
        inheritFrom(const ButtonOptions& prototype):
        - Params:   const ButtonOptions& prototype
        - Returns:  void
        - Desc:     Fills in every field the call site left alone from the
                    theme's prototype, and leaves the rest exactly as it was
                    set. Run when the element binds to its UILO, and again
                    whenever that UILO's theme changes.
    */
    void inheritFrom(const ButtonOptions& prototype) {
        m_outlineThickness.inherit(prototype.m_outlineThickness);
        m_outerPadding.inherit(prototype.m_outerPadding);
        m_innerPadding.inherit(prototype.m_innerPadding);
        m_rounding.inherit(prototype.m_rounding);
        m_colorRole.inherit(prototype.m_colorRole);
        m_gradientRole.inherit(prototype.m_gradientRole);
        m_outlineColorRole.inherit(prototype.m_outlineColorRole);
    }

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows the theme's default for this type.
    ButtonOptions& setOuterPadding(float px)   { m_outerPadding.set(px); return *this; }
    ButtonOptions& clearOuterPadding()         { m_outerPadding.clear(); return *this; }
    float  getOuterPadding()     const { return m_outerPadding.get().value_or(0.f); }

    // Space kept between this element's edge and the box its label sits in. Unset follows the theme's default for this type.
    ButtonOptions& setInnerPadding(float px)   { m_innerPadding.set(px); return *this; }
    ButtonOptions& clearInnerPadding()         { m_innerPadding.clear(); return *this; }
    float  getInnerPadding()     const { return m_innerPadding.get().value_or(0.f); }

    ButtonOptions& setColor(const Color& c)              { m_color = c; return *this; }
    ButtonOptions& setColorRole(const std::string& r)    { m_colorRole.set(r); return *this; }
    ButtonOptions& setGradient(const Gradient& g)        { m_gradient = g; return *this; }
    ButtonOptions& setGradientRole(const std::string& r) { m_gradientRole.set(r); return *this; }
    ButtonOptions& setRounding(float r)                  { m_rounding.set(r); return *this; }
    ButtonOptions& inheritRounding(const std::optional<float>& own, float fallback);

    // Outline: a border drawn inside the element's bounds, so turning one on
    // never changes the space the element takes.
    ButtonOptions& setOutlineColor(const Color& c)           { m_outlineColor = c; return *this; }
    ButtonOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole.set(r); return *this; }
    ButtonOptions& setOutlineThickness(float px)             { m_outlineThickness.set(px); return *this; }

    ButtonOptions& setLabel(Text* t)                         { m_label = t; return *this; }

    Color              getColor()        const { return m_color; }
    const std::string& getColorRole()    const { return m_colorRole.get(); }
    const Gradient&    getGradient()     const { return m_gradient; }
    const std::string& getGradientRole() const { return m_gradientRole.get(); }
    float              getRounding()     const;
    const std::optional<float>& getRoundingOpt()      const { return m_rounding.get(); }
    float                       getRoundingFallback() const { return m_roundingFallback; }

    Color              getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole.get(); }
    float              getOutlineThickness() const { return m_outlineThickness.get(); }

    Text* getLabel() const { return m_label; }

private:
    Themed<std::optional<float>> m_outerPadding;
    Themed<std::optional<float>> m_innerPadding;
    Color       m_color = Color{0, 0, 0, 0};
    Themed<std::string> m_colorRole;
    Gradient    m_gradient;
    Themed<std::string> m_gradientRole;

    Themed<std::optional<float>> m_rounding;
    float                m_roundingFallback = 0.f;

    Color       m_outlineColor = Color::Transparent;
    Themed<std::string> m_outlineColorRole;
    Themed<float> m_outlineThickness {0.f};

    Text* m_label = nullptr;
    // The theme role this was constructed with; resolved at bind time.
    std::string m_themeRole;
};


/*
    inheritRounding(const std::optional<float>& own, float fallback):
    - Params:   const std::optional<float>& own, float fallback
    - Returns:  ButtonOptions&
    - Desc:     Takes a rounding straight from a composite widget. `own` is
                whatever the widget was told, empty when nothing, and `fallback`
                is that widget's own default for when the theme is silent too.
                Passing it through unresolved, rather than as a number the
                widget already worked out, is what lets the theme keep reaching
                this element after it has been built.
    - An empty `own` leaves the field following the theme rather than pinning it
      empty, so a themed radius still reaches the part.
*/
inline ButtonOptions& ButtonOptions::inheritRounding(
    const std::optional<float>& own,
    float fallback
) {
    /* Only an actual value counts as a setting. Marking the field explicit
       when `own` is empty would say "the widget chose no rounding", which the
       theme must then leave alone -- and every inner part of every composite
       widget would silently stop following it. */
    if (own) m_rounding.set(own);
    m_roundingFallback = fallback;
    return *this;
}


/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius, resolved in three steps: the value this element
                was given, then the active Theme's, then the fallback carried by
                inheritRounding. Resolved on every read rather than cached, so
                changing the Theme restyles a button already on screen.
*/
inline float ButtonOptions::getRounding() const {
    return m_rounding.get().value_or(m_roundingFallback);
}


/*
    Button:
    - Desc:     A clickable Row. Being a Row rather than an Interactible is
                deliberate: a button has no state to keep between clicks, and it
                means the label goes in as an ordinary child, so anything else
                can be added beside it -- which is how Dropdown builds its
                header and arrow. It claims a press whether or not a callback
                was attached, so a click never falls through to the panel
                behind, and it asks for the hand cursor whenever the pointer is
                over it.
    - ButtonOptions are pushed down into the underlying RowOptions on every
      draw, so a handler that mutates getOptions() directly is reflected on the
      next frame without calling setOptions().
*/
class Button : public Row {
public:
    void applyTheme(const Theme& theme) override {
        m_buttonOptions.inheritFrom(theme.cascade<ButtonOptions>(m_buttonOptions.getThemeRole()));
        Element::applyTheme(theme);
    }

    float getOuterPadding() const override { return m_buttonOptions.getOuterPadding(); }
    float getInnerPadding() const override { return m_buttonOptions.getInnerPadding(); }

    explicit Button(
        Modifier modifier,
        ButtonOptions options = {},
        const std::string& name = ""
    );

    const ButtonOptions& getOptions() const { return m_buttonOptions; }
    ButtonOptions&       getOptions()       { return m_buttonOptions; }
    void                 setOptions(const ButtonOptions& opts);

    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkRightClick(const Vec2f& mousePosition) override;
    bool checkHover(const Vec2f& mousePosition) override;
    bool checkScroll(
        const Vec2f& mousePosition,
        float delta,
        bool precise = false,
        bool momentum = false
    ) override;

    void render() override;

protected:
    bool claimsPointerEvents() const override { return true; }

private:
    ButtonOptions m_buttonOptions;
};

}
