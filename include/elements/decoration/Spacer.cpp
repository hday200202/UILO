#include "Spacer.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"

namespace uilo {

/*
    Spacer(Modifier modifier, SpacerOptions options, const std::string& name):
    - Params:   Modifier modifier, SpacerOptions options,
                const std::string& name
    - Returns:  Spacer
    - Desc:     Constructs a spacer from a modifier and its options, and tags it
                as a Spacer so layout and the web bridge can identify it.
*/
Spacer::Spacer(
    Modifier modifier,
    SpacerOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name = name;
    m_type = ElementType::Spacer;
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Resolves the spacer's own bounds inside the slot its parent gave
                it. There is no state to advance, so the frame time is unused.
*/
void Spacer::update(Rectf& parentBounds, float dt) {
    resize(parentBounds);
    (void)dt;
}


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the spacer's fill and outline, and nothing at all in the
                usual case where it has neither. An outline with no fill is a
                frame, so a transparent spacer still draws when one is set. The
                dirty flag is cleared on every path, including the early ones,
                so a spacer that has nothing to draw does not keep asking to be
                redrawn.
*/
void Spacer::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }
    if (!m_uiloRef)               { m_dirty = false; return; }

    const Color c  = resolveColor(m_options.getColorRole(), m_options.getColor());
    const Color ol = resolveColor(m_options.getOutlineColorRole(),
                                  m_options.getOutlineColor());
    const float scale = m_uiloRef->getScale();
    const float olT   = m_options.getOutlineThickness() * scale;

    if (c.a == 0 && (olT <= 0.f || ol.a == 0)) { m_dirty = false; return; }

    const float r = m_options.getRounding() * scale;
    auto& renderer = m_uiloRef->getRenderer();
    if (r <= 0.f)
        renderer.draw(Rect{m_bounds.position, m_bounds.size, c, ol, olT});
    else
        renderer.draw(RoundedRect{m_bounds.position, m_bounds.size, r, 8, c, ol, olT});

    m_dirty = false;
}

}
