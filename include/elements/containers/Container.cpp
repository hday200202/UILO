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

    for (auto& child : children)
        m_children.push_back(child);
}

bool Container::checkLeftClick(const Vec2f& mousePosition) {
    bool childClicked = false;

    for (auto& child : m_children)
        if (child->getBounds().intersects(mousePosition))
            childClicked |= child->checkLeftClick(mousePosition);

    if (!childClicked && m_bounds.intersects(mousePosition)) {
        if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()();
        return true;
    }

    return childClicked;
}

bool Container::checkRightClick(const Vec2f& mousePosition) {
    bool childClicked = false;

    for (auto& child : m_children)
        if (child->getBounds().intersects(mousePosition))
            childClicked |= child->checkRightClick(mousePosition);

    if (!childClicked && m_bounds.intersects(mousePosition)) {
        if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()();
        return true;
    }

    return childClicked;
}

bool Container::checkHover(const Vec2f& mousePosition) {
    bool childHovered = false;

    for (auto& child : m_children)
        if (child->getBounds().intersects(mousePosition))
            childHovered |= child->checkHover(mousePosition);

    if (!childHovered && m_bounds.intersects(mousePosition)) {
        if (!m_hovered && m_modifier.getOnHover())
            m_modifier.getOnHover()();
        m_hovered = true;
        return true;
    }

    m_hovered = false;
    return childHovered;
}

bool Container::checkScroll(const Vec2f& mousePosition, float delta) {
    bool childScrolled = false;

    for (auto& child : m_children)
        if (child->getBounds().intersects(mousePosition))
            childScrolled |= child->checkScroll(mousePosition, delta);

    if (!childScrolled && m_bounds.intersects(mousePosition)) {
        if (m_modifier.getOnScroll()) m_modifier.getOnScroll()(delta);
        return true;
    }

    return childScrolled;
}

void Container::addElement(Element* element) { m_children.push_back(element); }

void Container::pruneChildren() {
    m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(),
            [](Element* e) { return e->m_markedForDeletion; }),
        m_children.end());
}

void Container::setUilo(UILO& uiloRef) {
    Element::setUilo(uiloRef);
    for (auto& child : m_children)
        child->setUilo(uiloRef);
}

}