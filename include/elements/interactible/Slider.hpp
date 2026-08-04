#pragma once

#include <optional>

#include <functional>
#include <SDL3/SDL.h>

#include "Interactible.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    ValueChangedFuncPtr:
    - Desc:     Storage signature for a slider's value callback. Fired whenever
                the value actually moves, not on every input event, so a handler
                can be expensive without being called on a frame where nothing
                changed.
*/
using ValueChangedFuncPtr = std::function<void(float)>;


/*
    ThumbShape:
    - Desc:     How the slider's handle is drawn -- a circle, or a rectangle
                whose corner radius comes from setThumbRounding.
*/
enum class ThumbShape       { Circle, Rect };


/*
    SliderOrientation:
    - Desc:     Which axis the slider runs along. Horizontal fills left to
                right; Vertical fills bottom to top, so a level or volume
                control reads the way one is expected to.
*/
enum class SliderOrientation { Horizontal, Vertical };

/*
    SliderOptions:
    - Desc:     Everything a Slider draws and the range it works over: the
                track, the filled portion, the thumb, the value range and step,
                and the orientation. Colors come as a literal plus a role, where
                the role wins when it resolves against the active Palette and
                the literal is the fallback.
    - setTrackThickness is read two ways. A value of 1 or below is a fraction of
      the cross-axis size, so the track scales with the element, and anything
      above 1 is a pixel thickness.
    - A step of 0 is continuous; anything above snaps the value to that
      increment.
*/
class SliderOptions {
public:
    SliderOptions() = default;

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows
    // Theme::setOuterPadding().
    SliderOptions& setOuterPadding(float px)   { m_outerPadding = px; return *this; }
    SliderOptions& clearOuterPadding()         { m_outerPadding.reset(); return *this; }
    float  getOuterPadding()     const { return Theme::resolveOuterPadding(m_outerPadding, 0.f); }
    
    SliderOptions& setTrackColor(Color c)               { m_trackColor = c;         return *this; }
    SliderOptions& setTrackColorRole(const std::string& r) { m_trackColorRole = r;  return *this; }
    SliderOptions& setFillColor(Color c)                { m_fillColor = c;          return *this; }
    SliderOptions& setFillColorRole(const std::string& r) { m_fillColorRole = r;    return *this; }
    SliderOptions& setThumbColor(Color c)               { m_thumbColor = c;         return *this; }
    SliderOptions& setThumbColorRole(const std::string& r) { m_thumbColorRole = r;  return *this; }
    SliderOptions& setThumbShape(ThumbShape s)              { m_thumbShape = s;         return *this; }
    SliderOptions& setTrackThickness(float t)               { m_trackThickness = t;     return *this; }
    SliderOptions& setTrackRounding(float r)                { m_trackRounding = r;      return *this; }
    SliderOptions& setThumbSize(float w, float h = 0.f)     { m_thumbSize = {w, h};     return *this; }
    SliderOptions& setThumbRounding(float r)                { m_thumbRounding = r;      return *this; }
    SliderOptions& setRange(float mn, float mx)             { m_min = mn; m_max = mx;   return *this; }
    SliderOptions& setStep(float s)                         { m_step = s;               return *this; }
    SliderOptions& setInvertScroll(bool invert)             { m_invertScroll = invert;  return *this; }
    SliderOptions& setDefaultValue(float v)                 { m_defaultValue = v; m_hasDefault = true; return *this; }
    SliderOptions& setOnValueChanged(ValueChangedFuncPtr f) { m_onValueChanged = std::move(f); return *this; }
    SliderOptions& setOrientation(SliderOrientation o)      { m_orientation = o;        return *this; }

    Color            getTrackColor()      const { return m_trackColor; }
    const std::string& getTrackColorRole() const { return m_trackColorRole; }
    Color            getFillColor()       const { return m_fillColor; }
    const std::string& getFillColorRole()  const { return m_fillColorRole; }
    Color            getThumbColor()      const { return m_thumbColor; }
    const std::string& getThumbColorRole() const { return m_thumbColorRole; }
    ThumbShape           getThumbShape()      const { return m_thumbShape; }
    float                getTrackThickness()  const { return m_trackThickness; }
    float                getTrackRounding()   const;
    Vec2f         getThumbSize()       const { return m_thumbSize; }
    float                getThumbRounding()   const;
    float                getMin()             const { return m_min; }
    float                getMax()             const { return m_max; }
    float                getStep()            const { return m_step; }
    bool                 getInvertScroll()    const { return m_invertScroll; }
    float                getDefaultValue()    const { return m_hasDefault ? m_defaultValue : m_min; }
    bool                 hasDefault()         const { return m_hasDefault; }
    SliderOrientation    getOrientation()     const { return m_orientation; }
    const ValueChangedFuncPtr& getOnValueChanged() const { return m_onValueChanged; }

private:
    std::optional<float> m_outerPadding;
    Color            m_trackColor      = Color{60, 60, 60, 255};
    std::string          m_trackColorRole = "panelAlt";
    Color            m_fillColor       = Color::White;
    std::string          m_fillColorRole  = "accent";
    Color            m_thumbColor      = Color::White;
    std::string          m_thumbColorRole = "text";
    ThumbShape           m_thumbShape      = ThumbShape::Circle;
    float                m_trackThickness  = 0.25f;   /* <=1: fraction of cross-axis size, >1: pixels */
    std::optional<float>                m_trackRounding;   /* corner radius of the track bar (px) */
    Vec2f            m_thumbSize       = {8.f, 0.f};
    std::optional<float>                m_thumbRounding;   /* corner radius of thumb rect */
    float                m_min             = 0.f;
    float                m_max             = 1.f;
    float                m_step            = 0.f;   /* 0 = continuous; >0 = discrete snap increment */
    bool                 m_invertScroll    = false;
    float                m_defaultValue    = 0.f;
    bool                 m_hasDefault      = false;
    SliderOrientation    m_orientation     = SliderOrientation::Horizontal;
    ValueChangedFuncPtr  m_onValueChanged;
};

/*
    getTrackRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of the track bar, resolved in three steps: the
                value this slider was given, then the active Theme's, then 0.
*/
inline float SliderOptions::getTrackRounding() const {
    return Theme::resolveRounding(m_trackRounding, 0.f);
}


/*
    getThumbRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of a Rect thumb, resolved the same three ways.
                Ignored by a Circle thumb.
*/
inline float SliderOptions::getThumbRounding() const {
    return Theme::resolveRounding(m_thumbRounding, 0.f);
}


/*
    Slider:
    - Desc:     A draggable value control. Clicking anywhere on the track jumps
                the value there and begins a drag, so a press and a drag are the
                same gesture; double-clicking restores the configured default.
                The wheel adjusts it too, accumulating sub-step motion so a
                stepped slider still responds to a slow trackpad rather than
                ignoring deltas too small to cross an increment.
    - The thumb is inset from both ends by its own half-size, so its edge stops
      at the track's edge instead of hanging past it, and the usable travel is
      measured against that inset span.
*/
class Slider : public Interactible {
public:
    float getOuterPadding() const override { return m_options.getOuterPadding(); }

    Slider(
        Modifier modifier,
        SliderOptions options = {},
        const std::string& name = ""
    );

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    bool checkHover(const Vec2f& mousePosition) override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkScroll(
        const Vec2f& mousePosition,
        float delta,
        bool precise = false,
        bool momentum = false
    ) override;

    void onDeactivate() override;

    void  setValue(float value);
    float getValue() const { return m_value; }

    const SliderOptions& getOptions() const { return m_options; }
    SliderOptions&       getOptions()       { return m_options; }
    void                 setOptions(const SliderOptions& opts) { m_options = opts; }

private:
    float valueFromMouseX(float mouseX) const;
    float valueFromMouseY(float mouseY) const;
    void  applyValue(float raw);
    float resolveThumbHalfWidth()  const;
    float resolveThumbHalfHeight() const;

    SliderOptions m_options;
    float m_value    = 0.f;
    float m_scrollAccum = 0.f;
    bool  m_dragging = false;
    uint64_t m_lastClickMs = 0;
};

}
