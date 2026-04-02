#include "Spacer.hpp"

namespace uilo {

Spacer::Spacer(Modifier modifier, const std::string& name) {
    m_modifier  = modifier;
    m_name      = name;
    m_type      = ElementType::Spacer;
}

void Spacer::update(Bounds& parentBounds, float) { resize(parentBounds); }

void Spacer::render(Renderer&) {}

}
