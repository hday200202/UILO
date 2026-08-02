#pragma once

#include <SDL3/SDL.h>
#include "../Element.hpp"

namespace uilo {

/*
    Interactible:
    - Desc:     Base class for elements that carry state between clicks --
                Dropdown, Slider, Knob, Textbox and the like. Clicking one makes
                it the single active interactible through
                UILO::setCurrInteractible(); clicking a different one, or empty
                space, calls onDeactivate() on the previous holder so it can
                close itself or release focus. Keyboard and text events are
                routed only to the active one.
    - Button is deliberately not one of these -- it inherits Row directly and
      has no open or focused state to keep.
    - Every Interactible claims pointer events whether or not a Modifier
      callback was attached, so a press over a slider or a textbox is consumed
      rather than falling through to whatever is behind it.
*/
/*
    isShortcutModifier(bool ctrl, bool gui):
    - Params:   bool ctrl, bool gui
    - Returns:  bool
    - Desc:     Whether the modifier held is the one that means "this is an
                editing shortcut" -- copy, cut, paste, select-all. On macOS
                Command and Control are accepted interchangeably, since a Mac
                user reaches for Command and a user coming from anywhere else
                reaches for Control. Elsewhere only Control counts: Super is the
                window manager's key, not the application's.
*/
inline bool isShortcutModifier(bool ctrl, bool gui) {
#if defined(__APPLE__)
    return ctrl || gui;
#else
    (void)gui;
    return ctrl;
#endif
}


class Interactible : public Element {
public:
    Interactible() = default;

    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkRightClick(const Vec2f& mousePosition) override;

    // Called when another interactible, or empty space, is clicked. Override to
    // close a popup, release focus, and so on.
    virtual void onDeactivate() {}

    // Called by UILO's event routing while this is the active interactible.
    // Override in anything that takes typing.
    virtual void handleTextInput(char32_t /* unicode */) {}
    // `gui` is Command on macOS and Super elsewhere. It is reported separately
    // from `ctrl` rather than folded into it because a terminal needs to tell
    // them apart: Ctrl+C is an interrupt there, and only Cmd+C is a copy.
    virtual void handleKeyInput(
        SDL_Keycode /* key */,
        bool /* shift */,
        bool /* ctrl */,
        bool /* gui */
    ) {}

    // Return true from anything that consumes IME or text-input events. UILO
    // uses this to toggle SDL_StartTextInput as focus moves.
    virtual bool wantsTextInput() const { return false; }

    // A scroll over most interactibles moves a value, not a view -- a slider or
    // a knob would keep turning on its own if a flick were allowed to coast.
    // The ones that do scroll a view (Textbox, Terminal) turn this back on.
    bool wantsScrollMomentum() const override { return false; }

protected:
    bool claimsPointerEvents() const override { return true; }
};

}
