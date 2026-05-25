#pragma once

#include <functional>
#include <SFML/Graphics.hpp>

#include "Interactible.hpp"

namespace uilo {

using ValueChangedFuncPtr = std::function<void(float)>;

enum class ThumbShape       { Circle, Rect };
enum class SliderOrientation { Horizontal, Vertical };

class SliderOptions {
public:
    SliderOptions() = default;
    
    SliderOptions& setTrackColor(sf::Color c)               { m_trackColor = c;         return *this; }
    SliderOptions& setFillColor(sf::Color c)                { m_fillColor = c;          return *this; }
    SliderOptions& setThumbColor(sf::Color c)               { m_thumbColor = c;         return *this; }
    SliderOptions& setThumbShape(ThumbShape s)              { m_thumbShape = s;         return *this; }
    SliderOptions& setTrackThickness(float t)               { m_trackThickness = t;     return *this; }
    SliderOptions& setTrackRounding(float r)                { m_trackRounding = r;      return *this; }
    SliderOptions& setThumbSize(float w, float h = 0.f)     { m_thumbSize = {w, h};     return *this; }
    SliderOptions& setThumbRounding(float r)                { m_thumbRounding = r;      return *this; }
    SliderOptions& setRange(float mn, float mx)             { m_min = mn; m_max = mx;   return *this; }
    SliderOptions& setStep(float s)                         { m_step = s;               return *this; }
    SliderOptions& setOnValueChanged(ValueChangedFuncPtr f) { m_onValueChanged = std::move(f); return *this; }
    SliderOptions& setOrientation(SliderOrientation o)      { m_orientation = o;        return *this; }

    sf::Color            getTrackColor()      const { return m_trackColor; }
    sf::Color            getFillColor()       const { return m_fillColor; }
    sf::Color            getThumbColor()      const { return m_thumbColor; }
    ThumbShape           getThumbShape()      const { return m_thumbShape; }
    float                getTrackThickness()  const { return m_trackThickness; }
    float                getTrackRounding()   const { return m_trackRounding; }
    sf::Vector2f         getThumbSize()       const { return m_thumbSize; }
    float                getThumbRounding()   const { return m_thumbRounding; }
    float                getMin()             const { return m_min; }
    float                getMax()             const { return m_max; }
    float                getStep()            const { return m_step; }
    SliderOrientation    getOrientation()     const { return m_orientation; }
    const ValueChangedFuncPtr& getOnValueChanged() const { return m_onValueChanged; }

private:
    sf::Color            m_trackColor      = sf::Color(60, 60, 60);     // unfilled portion of track
    sf::Color            m_fillColor       = sf::Color::White;          // filled portion of track
    sf::Color            m_thumbColor      = sf::Color::White;          // draggable thumb
    ThumbShape           m_thumbShape      = ThumbShape::Circle;
    float                m_trackThickness  = 0.25f;                     // fraction of element height
    float                m_trackRounding   = 0.f;                       // corner radius of the track bar (px)
    sf::Vector2f         m_thumbSize       = {8.f, 0.f};                // width/height of thumb (height 0 = auto: element height; for Circle, width is diameter, 0 = auto)
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

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;

    bool checkHover(const sf::Vector2f& mousePosition) override;
    bool checkLeftClick(const sf::Vector2f& mousePosition) override;
    bool checkScroll(const sf::Vector2f& mousePosition, float delta) override;

    void onDeactivate() override;

    void  setValue(float value);
    float getValue() const { return m_value; }

    const SliderOptions& getOptions() const      { return m_options; }
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
