#pragma once

#include "../utils/Vec2.hpp"

namespace uilo {

enum class MouseButton { Left, Right, Middle };
enum class Key { Unknown, Backspace, Delete, Left, Right, Home, End, Enter, Escape };

class Input {
public:
    void poll() {}

    Vec2f mousePosition     = {0, 0};
    Vec2f mouseDelta        = {0, 0};
    float scrollVertical    = 0.f;
    float scrollHorizontal  = 0.f;

    bool mouseLeftDown      = false;
    bool mouseLeftPressed   = false;
    bool mouseLeftReleased  = false;

    bool mouseRightDown     = false;
    bool mouseRightPressed  = false;
    bool mouseRightReleased = false;

    bool mouseMiddleDown      = false;
    bool mouseMiddlePressed   = false;
    bool mouseMiddleReleased  = false;

    bool shiftHeld          = false;
    bool ctrlHeld           = false;
    bool altHeld            = false;

    Key keyPressed          = Key::Unknown;
    char textInput          = '\0';

    bool windowResized      = false;
    Vec2f windowSize        = {0, 0};
    bool windowClosed       = false;

    void resetFrame() {
        mouseLeftPressed    = false;
        mouseLeftReleased   = false;
        mouseRightPressed   = false;
        mouseRightReleased  = false;
        mouseMiddlePressed  = false;
        mouseMiddleReleased = false;
        mouseDelta          = {0, 0};
        scrollVertical      = 0.f;
        scrollHorizontal    = 0.f;
        keyPressed          = Key::Unknown;
        textInput           = '\0';
        windowResized       = false;
        windowClosed        = false;
    }
};

}