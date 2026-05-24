#pragma once

#include "../Element.hpp"

namespace uilo {

// Base class for stateful interactibles (Dropdown, Slider, Knob, TextBox, etc.).
// Excludes Button, which inherits Row directly and has no persistent open/focus state.
//
// When any Interactible is clicked, it becomes the single "active" interactible
// via UILO::setCurrInteractible(). When a different interactible (or empty space)
// is clicked, onDeactivate() is called on the previously active one so it can
// close itself, release focus, etc.
class Interactible : public Element {
public:
    Interactible() = default;

    bool checkLeftClick(const sf::Vector2f& mousePosition) override;
    bool checkRightClick(const sf::Vector2f& mousePosition) override;

    // Called by UILO when another interactible (or empty space) is clicked.
    // Override to close dropdowns, release focus, etc.
    virtual void onDeactivate() {}
};

}