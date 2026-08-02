#pragma once

#include "Interactible.hpp"

namespace uilo {

/*
    ResizerDir:
    - Desc:     Which neighbour a Resizer drags, and so which axis it works on.
                Left and Top resize the previous sibling, Right and Bottom the
                next one; Left/Right drag horizontally, Top/Bottom vertically.
*/
enum class ResizerDir { Left, Right, Top, Bottom };

/*
    ResizerOptions:
    - Desc:     Which neighbour the handle resizes, how wide its hit strip is,
                what it draws as, and the limits the drag is held within. The
                min, max and step limits are Dimensions, so they can be given in
                pixels or as a percent of the container -- a percent minimum
                keeps a panel usable at any window size. A step above 0
                quantises the drag to that increment.
    - The fill is transparent by default, so a resizer stays invisible until a
      hover handler colours it in.
*/
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
    Resizer:
    - Desc:     A drag handle placed inside a Row or Column beside the element
                it resizes. It occupies a strip for hit detection but is
                invisible to layout -- its siblings are placed as though it were
                not there, and it sits at the boundary between them -- so adding
                one never shifts the arrangement. It draws through UILO's post-
                render pass, which puts it on top of everything including the
                neighbours it straddles.
    - The target and the container bounds are pushed in by Row and Column during
      layout rather than looked up here, because only the parent knows which
      sibling is adjacent once invisible children have been skipped.
    - Double-clicking restores the target to the size it had when the handle
      first attached, which is why the original dimension is captured on the
      first setTarget rather than at construction -- there is no target yet
      then.
*/
class Resizer : public Interactible {
public:
    explicit Resizer(
        Modifier modifier = {},
        ResizerOptions options = {},
        const std::string& name = ""
    );

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    bool checkHover(const Vec2f& mousePosition) override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    void onDeactivate() override;

    // Both are pushed in by Row and Column during layout.
    void setTarget(Element* t);
    void setContainerBounds(Rectf b) { m_containerBounds = b; }

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

    // Last left-click timestamp in SDL ticks, for double-click detection.
    // 0 means no click yet, which is deliberately not a valid previous click.
    uint64_t m_lastClickMs = 0;
};


/*
    setTarget(Element* t):
    - Params:   Element* t
    - Returns:  void
    - Desc:     Attaches the element this handle drags. The target's declared
                size is captured the first time a real one arrives, so a double-
                click can restore it later; the capture is guarded because
                layout calls this every frame, and re-reading the size after a
                drag would make "restore" mean "whatever it was last frame".
*/
inline void Resizer::setTarget(Element* t) {
    if (t && t != m_target && !m_haveOriginalSize) {
        m_originalWidth    = t->getModifier().getWidth();
        m_originalHeight   = t->getModifier().getHeight();
        m_haveOriginalSize = true;
    }
    m_target = t;
}

}
