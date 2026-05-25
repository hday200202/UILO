#pragma once

#include "Interactible.hpp"

namespace uilo {

enum class ResizerDir { Left, Right, Top, Bottom };

class ResizerOptions {
public:
    ResizerOptions& setDirection(ResizerDir d)      { m_direction       = d; return *this; }
    ResizerOptions& setThickness(float t)           { m_thickness       = t; return *this; }
    ResizerOptions& setColor(sf::Color c)           { m_color           = c; return *this; }
    ResizerOptions& setResizeWidthMin(Dimension d)  { m_resizeWidthMin  = d; return *this; }
    ResizerOptions& setResizeWidthMax(Dimension d)  { m_resizeWidthMax  = d; return *this; }
    ResizerOptions& setResizeHeightMin(Dimension d) { m_resizeHeightMin = d; return *this; }
    ResizerOptions& setResizeHeightMax(Dimension d) { m_resizeHeightMax = d; return *this; }

    ResizerDir getDirection()       const { return m_direction; }
    float      getThickness()       const { return m_thickness; }
    sf::Color  getColor()           const { return m_color; }
    Dimension  getResizeWidthMin()  const { return m_resizeWidthMin; }
    Dimension  getResizeWidthMax()  const { return m_resizeWidthMax; }
    Dimension  getResizeHeightMin() const { return m_resizeHeightMin; }
    Dimension  getResizeHeightMax() const { return m_resizeHeightMax; }

private:
    ResizerDir m_direction       = ResizerDir::Right;
    float      m_thickness       = 8.f;
    sf::Color  m_color           = sf::Color::Transparent;
    Dimension  m_resizeWidthMin  = {0.f,       false};
    Dimension  m_resizeWidthMax  = {100000.f,  false};
    Dimension  m_resizeHeightMin = {0.f,       false};
    Dimension  m_resizeHeightMax = {100000.f,  false};
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

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;

    bool checkHover(const sf::Vector2f& mousePosition) override;
    bool checkLeftClick(const sf::Vector2f& mousePosition) override;
    void onDeactivate() override;

    // Called by Row/Column during layout
    void          setTarget(Element* t)          { m_target          = t; }
    void          setContainerBounds(sf::FloatRect b) { m_containerBounds = b; }

    Element*   getTarget()    const { return m_target; }
    ResizerDir getDirection() const { return m_options.getDirection(); }
    float      getThickness() const { return m_options.getThickness(); }

private:
    ResizerOptions m_options;
    Element*       m_target           = nullptr;
    sf::FloatRect  m_containerBounds;
    bool           m_dragging         = false;
    sf::Vector2f   m_dragStart;
    float          m_dragStartW       = 0.f;
    float          m_dragStartH       = 0.f;
};

}
