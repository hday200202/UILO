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

bool Container::checkLeftClick(const sf::Vector2f& mousePosition) {
    bool childClicked = false;

    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            childClicked |= child->checkLeftClick(mousePosition);

    if (!childClicked && m_bounds.contains(mousePosition)) {
        if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()();
        return true;
    }

    return childClicked;
}

bool Container::checkRightClick(const sf::Vector2f& mousePosition) {
    bool childClicked = false;

    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            childClicked |= child->checkRightClick(mousePosition);

    if (!childClicked && m_bounds.contains(mousePosition)) {
        if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()();
        return true;
    }

    return childClicked;
}

bool Container::checkHover(const sf::Vector2f& mousePosition) {
    bool childHovered = false;

    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            childHovered |= child->checkHover(mousePosition);

    if (!childHovered && m_bounds.contains(mousePosition)) {
        if (m_modifier.getOnHover()) m_modifier.getOnHover()();
        return true;
    }

    return childHovered;
}

bool Container::checkScroll(const sf::Vector2f& mousePosition, float delta) {
    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta)) return true;

    if (m_bounds.contains(mousePosition) && m_modifier.getOnScroll()) {
        m_modifier.getOnScroll()(delta);
        return true;
    }

    return false;
}

void Container::addElement(Element* element) {m_children.push_back(element); }

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

}