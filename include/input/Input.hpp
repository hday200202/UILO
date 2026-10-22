#pragma once

/*
    Input.hpp:
    - Desc: The key, mouse-button and modifier types UILO's input API speaks.
            They are VALO's -- the window UILO reads input from is a
            valo::Window -- re-exported under uilo:: so application code names
            them the same way it names everything else, and never needs SDL.
    - Key values are physical key positions (USB HID scancodes), so a binding
      stays on the same key across keyboard layouts. Where a key's meaning
      depends on the layout -- Ctrl+Z, Ctrl+C -- UILO passes the layout's
      character alongside it; see Interactible::handleKeyInput.
    - render/Window.hpp declares no SDL types beyond opaque forward
      declarations, so including it here keeps SDL out of UILO's headers.
*/

#include <render/Window.hpp>

namespace uilo {

using Key         = valo::Key;
using MouseButton = valo::MouseButton;
using KeyMod      = valo::KeyMod;

/* True when any of the bits in `wanted` are held in `mods`. */
inline bool hasMod(KeyMod mods, KeyMod wanted) {
    return valo::any(mods & wanted);
}

} // namespace uilo
