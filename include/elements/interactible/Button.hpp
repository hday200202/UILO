#pragma once

#include <optional>

#include "../containers/Row.hpp"
#include "../decoration/Text.hpp"
#include "../../utils/Theme.hpp"

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
    ButtonOptions() = default;

    ButtonOptions& setColor(const Color& c)              { m_color = c; return *this; }
    ButtonOptions& setColorRole(const std::string& r)    { m_colorRole = r; return *this; }
    ButtonOptions& setGradient(const Gradient& g)        { m_gradient = g; return *this; }
    ButtonOptions& setGradientRole(const std::string& r) { m_gradientRole = r; return *this; }
    ButtonOptions& setRounding(float r)                  { m_rounding = r; return *this; }
    ButtonOptions& inheritRounding(const std::optional<float>& own, float fallback);

    // Outline: a border drawn inside the element's bounds, so turning one on
    // never changes the space the element takes.
    ButtonOptions& setOutlineColor(const Color& c)           { m_outlineColor = c; return *this; }
    ButtonOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole = r; return *this; }
    ButtonOptions& setOutlineThickness(float px)             { m_outlineThickness = px; return *this; }

    ButtonOptions& setLabel(Text* t)                         { m_label = t; return *this; }

    Color              getColor()        const { return m_color; }
    const std::string& getColorRole()    const { return m_colorRole; }
    const Gradient&    getGradient()     const { return m_gradient; }
    const std::string& getGradientRole() const { return m_gradientRole; }
    float              getRounding()     const;
    const std::optional<float>& getRoundingOpt()      const { return m_rounding; }
    float                       getRoundingFallback() const { return m_roundingFallback; }

    Color              getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole; }
    float              getOutlineThickness() const { return m_outlineThickness; }

    Text* getLabel() const { return m_label; }

private:
    Color       m_color = Color{0, 0, 0, 0};
    std::string m_colorRole;
    Gradient    m_gradient;
    std::string m_gradientRole;

    std::optional<float> m_rounding;
    float                m_roundingFallback = 0.f;

    Color       m_outlineColor = Color::Transparent;
    std::string m_outlineColorRole;
    float       m_outlineThickness = 0.f;

    Text* m_label = nullptr;
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
*/
inline ButtonOptions& ButtonOptions::inheritRounding(
    const std::optional<float>& own,
    float fallback
) {
    m_rounding         = own;
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
    return Theme::resolveRounding(m_rounding, m_roundingFallback);
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
