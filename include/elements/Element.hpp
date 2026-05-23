#pragma once

#include <string>

#include "Modifier.hpp"

namespace uilo {

/*
    All element types finished or planned
*/
enum class ElementType {
    NONE,

    Column,
    Row,
    ScrollableColumn,
    ScrollableRow,
    Grid,

    Spacer,
    Text,
    Image,

    Button,
    Slider,
    Knob,
    TextBox,
};

class UILO;

class Element {
public:
    Element() = default;
    virtual ~Element() = default;

    sf::FloatRect getBounds() const;
    Modifier& getModifier();
    bool isDirty() const;
    void erase();
    
    virtual void setUILO(UILO& uiloRef);
    virtual void update(sf::FloatRect& parentBounds, float dt) = 0;
    virtual void render(sf::RenderTarget& target) = 0;
    virtual bool checkLeftClick(const sf::Vector2f& mousePosition);
    virtual bool checkRightClick(const sf::Vector2f& mousePosition);
    virtual bool checkHover(const sf::Vector2f& mousePosition);
    virtual bool checkScroll(const sf::Vector2f& mousePosition, float delta);
    void resize(const sf::FloatRect& parent);

    ElementType getType() const;

protected:
    UILO* m_uiloRef             = nullptr;
    std::string m_name          = "";

    sf::FloatRect m_bounds      = {{0.f, 0.f}, {0.f, 0.f}};
    sf::FloatRect m_pastBounds  = {{0.f, 0.f}, {0.f, 0.f}};

    bool m_dirty                = true;
    bool m_markedForDeletion    = false;
    bool m_hovered              = false;

    ElementType m_type          = ElementType::NONE;
    Modifier m_modifier         = Modifier();

    friend class UILO;
    friend class Container;
};

}