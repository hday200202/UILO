#pragma once

#include "../Element.hpp"

namespace uilo {

class SpacerOptions {
public:
    SpacerOptions() = default;

    SpacerOptions& setColor(const Color& c) { m_color    = c; return *this; }
    SpacerOptions& setColorRole(const std::string& r) { m_colorRole = r; return *this; }
    SpacerOptions& setRounding(float r)          { m_rounding = r; return *this; }

    Color getColor()    const { return m_color; }
    const std::string& getColorRole() const { return m_colorRole; }
    float     getRounding() const { return m_rounding; }

private:
    Color m_color    = Color{0,0,0,0};
    std::string m_colorRole;
    float     m_rounding = 0.f;
};

class Spacer : public Element {
public:
    explicit Spacer(Modifier modifier, SpacerOptions options = {}, const std::string& name = "");

    const SpacerOptions& getOptions() const        { return m_options; }
    SpacerOptions&       getOptions()              { return m_options; }
    void setOptions(const SpacerOptions& opts)      { m_options = opts; }

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

private:
    SpacerOptions m_options;
};

}