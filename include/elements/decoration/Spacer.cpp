#include "Spacer.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"

namespace uilo {

Spacer::Spacer(
    Modifier modifier,
    SpacerOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name = name;
}

void Spacer::update(Rectf& parentBounds, float dt) { resize(parentBounds); (void)dt; }

void Spacer::render() {
    // TODO: BGFX rendering for spacer background
    if (!m_uiloRef) { m_dirty = false; return; }
    const Color c = m_options.getColor();
    if (c.a == 0) { m_dirty = false; return; }
    const float scale = m_uiloRef->getScale();
    const float r = m_options.getRounding() * scale;
    auto& renderer = m_uiloRef->getRenderer();
    if (r <= 0.f)
        renderer.draw(Rect{m_bounds.position, m_bounds.size, c});
    else
        renderer.draw(RoundedRect{m_bounds.position, m_bounds.size, r, 8, c});
    m_dirty = false;
}

}