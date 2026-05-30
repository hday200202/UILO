#include "Container.hpp"

#include <algorithm>
#include "../../UILO.hpp"

namespace uilo {

Container::Container(
    Modifier modifier,
    contains children,
    const std::string& name
) {
    m_modifier = modifier;
    m_name = name;
    m_children.reserve(children.size());

    for (auto& child : children) m_children.push_back(child);
}

bool Container::isDirty() const {
    if (m_dirty) return true;
    for (auto* child : m_children) {
        if (child->isDirty()) return true;
    }
    return false;
}

bool Container::checkLeftClick(const Vec2f& mousePosition) {
    bool childClicked = false;

    for (auto& child : m_children) {
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getBounds().contains(mousePosition))
            childClicked |= child->checkLeftClick(mousePosition);
    }

    if (!childClicked && m_bounds.contains(mousePosition)) {
        if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()(this);
        return true;
    }

    return childClicked;
}

bool Container::checkRightClick(const Vec2f& mousePosition) {
    bool childClicked = false;

    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            childClicked |= child->checkRightClick(mousePosition);

    if (!childClicked && m_bounds.contains(mousePosition)) {
        if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()(this);
        return true;
    }

    return childClicked;
}

bool Container::checkHover(const Vec2f& mousePosition) {
    // Recurse into ALL children unconditionally so each one can fire its
    // own onHoverExit when the cursor leaves it. (Previously the parent
    // skipped children whose bounds did not contain the mouse, which
    // left their `m_hovered` flag stuck on `true` forever.)
    bool childHovered = false;

    for (auto& child : m_children) {
        if (child->getType() == ElementType::Resizer) continue;
        if (child->checkHover(mousePosition)) childHovered = true;
    }

    const bool inside = !childHovered && m_bounds.contains(mousePosition);

    if (inside && !m_hovered) {
        m_hovered = true; m_dirty = true;
        if (m_modifier.getOnHoverEnter()) m_modifier.getOnHoverEnter()(this);
    } else if (!inside && m_hovered) {
        m_hovered = false; m_dirty = true;
        if (m_modifier.getOnHoverExit()) m_modifier.getOnHoverExit()(this);
    }

    if (inside && m_uiloRef && m_modifier.getOnLeftClick())
        m_uiloRef->requestCursor(CursorType::Hand, 1);
    return inside;
}

bool Container::checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum) {
    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

    if (m_bounds.contains(mousePosition) && m_modifier.getOnScroll()) {
        m_modifier.getOnScroll()(this, delta);
        return true;
    }

    return false;
}

bool Container::checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum) {
    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

    if (m_bounds.contains(mousePosition) && m_modifier.getOnScroll()) {
        m_modifier.getOnScroll()(this, delta.y);
        return true;
    }

    return false;
}

bool Container::checkZoom(const Vec2f& mousePosition, float magnification) {
    // Last-wins: a Canvas drawn on top of siblings still wins. We iterate
    // forward but keep going through every child whose bounds contain the
    // cursor so deeper / later-drawn containers can claim the event.
    bool consumed = false;
    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkZoom(mousePosition, magnification)) consumed = true;
    return consumed;
}

void Container::addElement(Element* element) {
    if (!element) return;
    m_children.push_back(element);
    if (m_uiloRef) {
        element->setUILO(*m_uiloRef);
    }
    m_dirty = true;
}

void Container::pruneChildren() {
    m_children.erase(
        std::remove_if(
            m_children.begin(), m_children.end(), 
            [](Element* e) { return e->m_markedForDeletion; }
        ), m_children.end()
    );
}

void Container::setUILO(UILO& uiloRef) {
    Element::setUILO(uiloRef);
    for (auto& child : m_children)
        child->setUILO(uiloRef);
}

void Container::collectResizers(std::vector<Element*>& out) {
    for (auto* child : m_children) {
        if (child->getType() == ElementType::Resizer)
            out.push_back(child);
        else
            child->collectResizers(out);
    }
}

}