#pragma once

#include "Interactible.hpp"

namespace uilo {

enum class ResizerDir { Left, Right, Top, Bottom };

class ResizerOptions {
public:
    ResizerOptions& setDirection(ResizerDir d)      { m_direction       = d; return *this; }
    ResizerOptions& setThickness(float t)           { m_thickness       = t; return *this; }
    ResizerOptions& setColor(Color c)   { m_color = c; return *this; }
    ResizerOptions& setColorRole(const std::string& r) { m_colorRole = r; return *this; }
    ResizerOptions& setResizeWidthMin(Dimension d)  { m_resizeWidthMin  = d; return *this; }
    ResizerOptions& setResizeWidthMax(Dimension d)  { m_resizeWidthMax  = d; return *this; }
    ResizerOptions& setResizeWidthStep(Dimension d) { m_resizeWidthStep = d; return *this; }
    ResizerOptions& setResizeHeightMin(Dimension d) { m_resizeHeightMin = d; return *this; }
    ResizerOptions& setResizeHeightMax(Dimension d) { m_resizeHeightMax = d; return *this; }
    ResizerOptions& setResizeHeightStep(Dimension d){ m_resizeHeightStep = d; return *this; }

    ResizerDir getDirection()       const { return m_direction; }
    float      getThickness()       const { return m_thickness; }
    Color      getColor()           const { return m_color; }
    const std::string& getColorRole() const { return m_colorRole; }
    Dimension  getResizeWidthMin()  const { return m_resizeWidthMin; }
    Dimension  getResizeWidthMax()  const { return m_resizeWidthMax; }
    Dimension  getResizeWidthStep() const { return m_resizeWidthStep; }
    Dimension  getResizeHeightMin() const { return m_resizeHeightMin; }
    Dimension  getResizeHeightMax() const { return m_resizeHeightMax; }
    Dimension  getResizeHeightStep()const { return m_resizeHeightStep; }

private:
    ResizerDir m_direction       = ResizerDir::Right;
    float      m_thickness       = 8.f;
    Color   m_color = Color{0,0,0,0};
    std::string m_colorRole;
    Dimension  m_resizeWidthMin  = {0.f,       false};
    Dimension  m_resizeWidthMax  = {100000.f,  false};
    Dimension  m_resizeWidthStep = {0.f,       false};
    Dimension  m_resizeHeightMin = {0.f,       false};
    Dimension  m_resizeHeightMax = {100000.f,  false};
    Dimension  m_resizeHeightStep = {0.f,      false};
};

/*
    A Resizer is placed inside a Row or Column alongside the element it should resize.
    It occupies a physical strip (for hit detection) but is invisible to siblings —
    they are laid out as if the Resizer isn't there. The Resizer renders on top of
    everything via UILO's post-render pass.

    Direction tells which adjacent sibling to resize:
      Left / Top   → resizes the previous sibling
      Right / Bottom → resizes the next sibling
    The drag axis and cursor match the direction (Left/Right = horizontal, Top/Bottom = vertical).
*/
class Resizer : public Interactible {
public:
    explicit Resizer(Modifier modifier = {}, ResizerOptions options = {}, const std::string& name = "");

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    bool checkHover(const Vec2f& mousePosition) override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    void onDeactivate() override;

    // Called by Row/Column during layout
    void          setTarget(Element* t) {
        // Capture the target's original width/height the first time a real
        // target is attached, so double-click can restore it.
        if (t && t != m_target && !m_haveOriginalSize) {
            m_originalWidth    = t->getModifier().getWidth();
            m_originalHeight   = t->getModifier().getHeight();
            m_haveOriginalSize = true;
        }
        m_target = t;
    }
    void          setContainerBounds(Rectf b) { m_containerBounds = b; }

    Element*   getTarget()    const { return m_target; }
    ResizerDir getDirection() const { return m_options.getDirection(); }
    float      getThickness() const { return m_options.getThickness(); }

    const ResizerOptions& getOptions() const { return m_options; }
    ResizerOptions&       getOptions()       { return m_options; }

    bool isDragging() const { return m_dragging; }

private:
    ResizerOptions m_options;
    Element*       m_target           = nullptr;
    Rectf   m_containerBounds;
    bool    m_dragging         = false;
    Vec2f   m_dragStart;
    float          m_dragStartW       = 0.f;
    float          m_dragStartH       = 0.f;

    // Original size of the attached target (captured on first setTarget).
    Dimension m_originalWidth        = {};
    Dimension m_originalHeight       = {};
    bool      m_haveOriginalSize     = false;

    // Last left-click timestamp (SDL ticks, ms) for double-click detection.
    uint64_t  m_lastClickMs          = 0;
};

}
