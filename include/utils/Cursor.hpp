#pragma once

namespace uilo {

/*
    CursorType:
    - Desc: The mouse cursor shapes UILO can ask the platform for. Requests are
            made per frame and resolved by priority, so the shape follows whatever
            is under the pointer without anyone having to reset it on the way out.
    - Lives in its own header so Modifier can carry one without pulling in the
      renderer.
*/
enum class CursorType {
    Arrow,
    Hand,
    SizeHorizontal,
    SizeVertical,
    Text,
    Crosshair,
};

}
