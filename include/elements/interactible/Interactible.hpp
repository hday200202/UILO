#pragma once

#include <SDL3/SDL.h>
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

    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkRightClick(const Vec2f& mousePosition) override;

    // Called by UILO when another interactible (or empty space) is clicked.
    // Override to close dropdowns, release focus, etc.
    virtual void onDeactivate() {}

    // Called by UILO event routing for keyboard/text input.
    // Override in text-input interactibles (e.g. Textbox).
    virtual void handleTextInput(char32_t /* unicode */) {}
    virtual void handleKeyInput(SDL_Keycode /* key */, bool /* shift */, bool /* ctrl */) {}

    // Override and return true in interactibles that consume IME / text-input
    // events (Textbox). UILO uses this to toggle SDL_StartTextInput on focus.
    virtual bool wantsTextInput() const { return false; }
};

}