#pragma once

#include <optional>

#include "../Element.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    SpacerOptions:
    - Desc:     Everything a Spacer draws, which by default is nothing: the fill
                is transparent and there is no border, so a spacer is pure empty
                space. Giving it a colour turns it into a bar or a divider, and
                giving it an outline alone turns it into a frame. Colors come as
                a literal plus a role, where the role wins when it resolves
                against the active Palette and the literal is the fallback.
*/
class SpacerOptions {
public:
    SpacerOptions() = default;

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows
    // Theme::setOuterPadding().
    SpacerOptions& setOuterPadding(float px)   { m_outerPadding = px; return *this; }
    SpacerOptions& clearOuterPadding()         { m_outerPadding.reset(); return *this; }
    float  getOuterPadding()     const { return Theme::resolveOuterPadding(m_outerPadding, 0.f); }


    SpacerOptions& setColor(const Color& c)           { m_color = c; return *this; }
    SpacerOptions& setColorRole(const std::string& r) { m_colorRole = r; return *this; }
    SpacerOptions& setRounding(float r)               { m_rounding = r; return *this; }

    // Outline: a border drawn inside the element's bounds, so turning one on
    // never changes the space the element takes.
    SpacerOptions& setOutlineColor(const Color& c)           { m_outlineColor = c; return *this; }
    SpacerOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole = r; return *this; }
    SpacerOptions& setOutlineThickness(float px)             { m_outlineThickness = px; return *this; }

    Color              getColor()     const { return m_color; }
    const std::string& getColorRole() const { return m_colorRole; }
    float              getRounding()  const;

    Color              getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole; }
    float              getOutlineThickness() const { return m_outlineThickness; }

private:
    std::optional<float> m_outerPadding;
    Color                m_color = Color{0, 0, 0, 0};
    std::string          m_colorRole;
    std::optional<float> m_rounding;

    Color       m_outlineColor = Color::Transparent;
    std::string m_outlineColorRole;
    float       m_outlineThickness = 0.f;
};


/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius, resolved in three steps: the value this element
                was given, then the active Theme's, then 0. Resolved on every
                read rather than cached, so changing the Theme restyles a spacer
                already on screen.
    - Outline thickness is deliberately left unthemed, because a Spacer is a gap
      and a themed border would draw a line around every one of them.
*/
inline float SpacerOptions::getRounding() const {
    return Theme::resolveRounding(m_rounding, 0.f);
}


/*
    Spacer:
    - Desc:     Empty space in a layout, and the usual way to push siblings
                apart or hold a gap open. Takes part in layout like any other
                element -- a percent width shares the space left over, a pixel
                width is fixed -- but draws nothing unless it was given a fill
                or an outline. Carries no callbacks of its own, so it never
                swallows a click or hover meant for the container it sits in.
*/
class Spacer : public Element {
public:
    float getOuterPadding() const override { return m_options.getOuterPadding(); }

    explicit Spacer(
        Modifier modifier,
        SpacerOptions options = {},
        const std::string& name = ""
    );

    const SpacerOptions& getOptions() const { return m_options; }
    SpacerOptions&       getOptions()       { return m_options; }
    void                 setOptions(const SpacerOptions& opts) { m_options = opts; }

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

private:
    SpacerOptions m_options;
};

}
