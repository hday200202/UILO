#pragma once

#include <functional>
#include <SDL3/SDL.h>

#include "Interactible.hpp"

namespace uilo {

using KnobValueChangedFuncPtr = std::function<void(float)>;

// Direction the arc sweeps when going from start angle to end angle.
// Combined with the start/end angles this covers every DAW-style layout
// (e.g. start=135, end=45, Clockwise = sweep down/under-bottom;
//  start=135, end=45, CounterClockwise = sweep up/over-top).
enum class KnobArcDir { Clockwise, CounterClockwise };

class KnobOptions {
public:
    KnobOptions() = default;

    // Colors --------------------------------------------------------------
    KnobOptions& setBodyColor(Color c)        { m_bodyColor = c;       return *this; }
    KnobOptions& setBodyColorRole(const std::string& r) { m_bodyColorRole = r; return *this; }
    KnobOptions& setOutlineColor(Color c)     { m_outlineColor = c;    return *this; }
    KnobOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole = r; return *this; }
    KnobOptions& setTrackColor(Color c)       { m_trackColor = c;      return *this; }
    KnobOptions& setTrackColorRole(const std::string& r) { m_trackColorRole = r; return *this; }
    KnobOptions& setArcColor(Color c)         { m_arcColor = c;        return *this; }
    KnobOptions& setArcColorRole(const std::string& r) { m_arcColorRole = r; return *this; }
    KnobOptions& setIndicatorColor(Color c)   { m_indicatorColor = c;  return *this; }
    KnobOptions& setIndicatorColorRole(const std::string& r) { m_indicatorColorRole = r; return *this; }

    // Geometry ------------------------------------------------------------
    // Body radius is taken from the element bounds (min(w,h)/2). These
    // thicknesses are unscaled px and are multiplied by UILO scale at draw.
    KnobOptions& setOutlineThickness(float t) { m_outlineThickness = t; return *this; }
    KnobOptions& setArcThickness(float t)     { m_arcThickness = t;     return *this; }
    KnobOptions& setIndicatorThickness(float t){m_indicatorThickness = t;return *this; }
    // Gap (in px, unscaled) between body rim and inner edge of the arc.
    KnobOptions& setArcGap(float g)           { m_arcGap = g;           return *this; }
    // Indicator length as a fraction of body radius (1.0 = touches rim).
    KnobOptions& setIndicatorLength(float f)  { m_indicatorLength = f;  return *this; }
    // Indicator inner offset as a fraction of body radius (0.0 = from center).
    KnobOptions& setIndicatorInset(float f)   { m_indicatorInset = f;   return *this; }
    // Number of segments per full revolution used when tessellating arcs.
    // The actual segment count scales with the arc's sweep angle.
    KnobOptions& setSegments(int s)           { m_segments = s;         return *this; }

    // Arc layout (degrees, cartesian convention: 0=+x, 90=+y, etc.) -----
    KnobOptions& setStartAngle(float deg)     { m_startAngle = deg;     return *this; }
    KnobOptions& setEndAngle(float deg)       { m_endAngle = deg;       return *this; }
    KnobOptions& setArcDirection(KnobArcDir d){ m_arcDir = d;           return *this; }

    // Value behaviour -----------------------------------------------------
    KnobOptions& setRange(float mn, float mx) { m_min = mn; m_max = mx; return *this; }
    KnobOptions& setDefaultValue(float v)     { m_defaultValue = v; m_hasDefault = true; return *this; }
    KnobOptions& setStep(float s)             { m_step = s;             return *this; }
    // Pixels of vertical drag (after UI scale) required to traverse the
    // full value range.
    KnobOptions& setDragPixelsPerRange(float p){m_dragPixelsPerRange = p;return *this; }
    KnobOptions& setOnValueChanged(KnobValueChangedFuncPtr f) { m_onValueChanged = std::move(f); return *this; }

    Color getBodyColor()        const { return m_bodyColor; }
    const std::string& getBodyColorRole() const { return m_bodyColorRole; }
    Color getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole; }
    Color getTrackColor()       const { return m_trackColor; }
    const std::string& getTrackColorRole() const { return m_trackColorRole; }
    Color getArcColor()         const { return m_arcColor; }
    const std::string& getArcColorRole() const { return m_arcColorRole; }
    Color getIndicatorColor()   const { return m_indicatorColor; }
    const std::string& getIndicatorColorRole() const { return m_indicatorColorRole; }
    float getOutlineThickness() const { return m_outlineThickness; }
    float getArcThickness()     const { return m_arcThickness; }
    float getIndicatorThickness()const{ return m_indicatorThickness; }
    float getArcGap()           const { return m_arcGap; }
    float getIndicatorLength()  const { return m_indicatorLength; }
    float getIndicatorInset()   const { return m_indicatorInset; }
    int   getSegments()         const { return m_segments; }
    float getStartAngle()       const { return m_startAngle; }
    float getEndAngle()         const { return m_endAngle; }
    KnobArcDir getArcDirection()const { return m_arcDir; }
    float getMin()              const { return m_min; }
    float getMax()              const { return m_max; }
    float getDefaultValue()     const { return m_hasDefault ? m_defaultValue : m_min; }
    bool  hasDefault()          const { return m_hasDefault; }
    float getStep()             const { return m_step; }
    float getDragPixelsPerRange()const{ return m_dragPixelsPerRange; }
    const KnobValueChangedFuncPtr& getOnValueChanged() const { return m_onValueChanged; }

private:
    Color m_bodyColor          = Color{55, 58, 74};
    std::string m_bodyColorRole;
    Color m_outlineColor       = Color::Transparent;
    std::string m_outlineColorRole;
    Color m_trackColor         = Color{30, 32, 42};
    std::string m_trackColorRole;
    Color m_arcColor           = Color{151, 120, 206};
    std::string m_arcColorRole;
    Color m_indicatorColor     = Color::White;
    std::string m_indicatorColorRole;
    float m_outlineThickness   = 0.f;
    float m_arcThickness       = 4.f;
    float m_indicatorThickness = 2.f;
    float m_arcGap             = 3.f;
    float m_indicatorLength    = 0.75f;
    float m_indicatorInset     = 0.15f;
    int   m_segments           = 96;
    // Default sweep: typical DAW knob — lower-left to lower-right going
    // counterclockwise through the top (in screen coords where +y is down).
    float m_startAngle         = 135.f;
    float m_endAngle           = 45.f;
    KnobArcDir m_arcDir        = KnobArcDir::CounterClockwise;
    float m_min                = 0.f;
    float m_max                = 1.f;
    float m_defaultValue       = 0.f;
    bool  m_hasDefault         = false;
    float m_step               = 0.f;
    float m_dragPixelsPerRange = 150.f;
    KnobValueChangedFuncPtr m_onValueChanged;
};

class Knob : public Interactible {
public:
    Knob(Modifier modifier, KnobOptions options = {}, const std::string& name = "");

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    bool checkHover(const Vec2f& mousePosition) override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkScroll(const Vec2f& mousePosition, float delta, bool precise = false, bool momentum = false) override;

    void onDeactivate() override;

    void  setValue(float v);
    float getValue() const { return m_value; }

    const KnobOptions& getOptions() const { return m_options; }
    KnobOptions&       getOptions()       { return m_options; }
    void setOptions(const KnobOptions& o) { m_options = o; m_dirty = true; }

private:
    void  applyValue(float raw);
    // Total signed sweep from start to end along the chosen direction,
    // always in (0, 360]. 0 collapses to 360 to give a full ring.
    float sweepDegrees() const;
    // Returns the angle (degrees, cartesian) of the current value along
    // the configured arc.
    float angleForValue(float v) const;

    KnobOptions m_options;
    float m_value         = 0.f;
    float m_scrollAccum   = 0.f;
    bool  m_dragging      = false;
    float m_dragStartY    = 0.f;
    float m_dragStartVal  = 0.f;
    uint64_t m_lastClickMs = 0;
};

}
