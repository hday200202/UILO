#pragma once

#include <functional>
#include <SDL3/SDL.h>

#include "Interactible.hpp"

namespace uilo {

using ValueChangedFuncPtr = std::function<void(float)>;

enum class ThumbShape       { Circle, Rect };
enum class SliderOrientation { Horizontal, Vertical };

class SliderOptions {
public:
    SliderOptions() = default;
    
    SliderOptions& setTrackColor(Color c)               { m_trackColor = c;         return *this; }
    SliderOptions& setFillColor(Color c)                { m_fillColor = c;          return *this; }
    SliderOptions& setThumbColor(Color c)               { m_thumbColor = c;         return *this; }
    SliderOptions& setThumbShape(ThumbShape s)              { m_thumbShape = s;         return *this; }
    SliderOptions& setTrackThickness(float t)               { m_trackThickness = t;     return *this; }
    SliderOptions& setTrackRounding(float r)                { m_trackRounding = r;      return *this; }
    SliderOptions& setThumbSize(float w, float h = 0.f)     { m_thumbSize = {w, h};     return *this; }
    SliderOptions& setThumbRounding(float r)                { m_thumbRounding = r;      return *this; }
    SliderOptions& setRange(float mn, float mx)             { m_min = mn; m_max = mx;   return *this; }
    SliderOptions& setStep(float s)                         { m_step = s;               return *this; }
    SliderOptions& setOnValueChanged(ValueChangedFuncPtr f) { m_onValueChanged = std::move(f); return *this; }
    SliderOptions& setOrientation(SliderOrientation o)      { m_orientation = o;        return *this; }

    Color            getTrackColor()      const { return m_trackColor; }
    Color            getFillColor()       const { return m_fillColor; }
    Color            getThumbColor()      const { return m_thumbColor; }
    ThumbShape           getThumbShape()      const { return m_thumbShape; }
    float                getTrackThickness()  const { return m_trackThickness; }
    float                getTrackRounding()   const { return m_trackRounding; }
    Vec2f         getThumbSize()       const { return m_thumbSize; }
    float                getThumbRounding()   const { return m_thumbRounding; }
    float                getMin()             const { return m_min; }
    float                getMax()             const { return m_max; }
    float                getStep()            const { return m_step; }
    SliderOrientation    getOrientation()     const { return m_orientation; }
    const ValueChangedFuncPtr& getOnValueChanged() const { return m_onValueChanged; }

private:
    Color            m_trackColor      = Color{60, 60, 60, 255};
    Color            m_fillColor       = Color::White;
    Color            m_thumbColor      = Color::White;
    ThumbShape           m_thumbShape      = ThumbShape::Circle;
    float                m_trackThickness  = 0.25f;                     // fraction of element height
    float                m_trackRounding   = 0.f;                       // corner radius of the track bar (px)
    Vec2f            m_thumbSize       = {8.f, 0.f};
    float                m_thumbRounding   = 0.f;                       // corner radius of thumb rect
    float                m_min             = 0.f;
    float                m_max             = 1.f;
    float                m_step            = 0.f;                       // 0 = continuous; >0 = discrete snap increment
    SliderOrientation    m_orientation     = SliderOrientation::Horizontal;
    ValueChangedFuncPtr  m_onValueChanged;
};

class Slider : public Interactible {
public:
    Slider(Modifier modifier, SliderOptions options = {}, const std::string& name = "");

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    bool checkHover(const Vec2f& mousePosition) override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkScroll(const Vec2f& mousePosition, float delta) override;

    void onDeactivate() override;

    void  setValue(float value);
    float getValue() const { return m_value; }

    const SliderOptions& getOptions() const      { return m_options; }
    SliderOptions&       getOptions()            { return m_options; }
    void setOptions(const SliderOptions& opts)   { m_options = opts; }

private:
    float valueFromMouseX(float mouseX) const;
    float valueFromMouseY(float mouseY) const;
    void  applyValue(float raw);
    float resolveThumbHalfWidth()  const;
    float resolveThumbHalfHeight() const;

    SliderOptions m_options;
    float m_value    = 0.f;
    bool  m_dragging = false;
};

}
