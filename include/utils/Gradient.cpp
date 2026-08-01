#include "Gradient.hpp"
#include "../Palette.hpp"

namespace uilo {

/*
    vertical(GradientColor top, GradientColor bottom):
    - Params:   GradientColor top, GradientColor bottom
    - Returns:  Gradient
    - Desc:     A gradient running top to bottom: both top corners take the
                first colour and both bottom corners the second.
*/
Gradient Gradient::vertical(GradientColor top, GradientColor bottom) {
    return Gradient(top, top, bottom, bottom);
}

/*
    horizontal(GradientColor left, GradientColor right):
    - Params:   GradientColor left, GradientColor right
    - Returns:  Gradient
    - Desc:     A gradient running left to right: both left corners take the
                first colour and both right corners the second.
*/
Gradient Gradient::horizontal(GradientColor left, GradientColor right) {
    return Gradient(left, right, left, right);
}

/*
    resolve(const Palette& palette, Color out[4]):
    - Params:   const Palette& palette, Color out[4]
    - Returns:  none
    - Desc:     Resolves the four corner stops into concrete colours, each
                through its own palette role where it has one. Fills `out` in
                top-left, top-right, bottom-left, bottom-right order, which is
                the order the renderer expects its vertex colours in.
*/
void Gradient::resolve(const Palette& palette, Color out[4]) const {
    out[0] = palette.resolve(topLeft.role,     topLeft.color);
    out[1] = palette.resolve(topRight.role,    topRight.color);
    out[2] = palette.resolve(bottomLeft.role,  bottomLeft.color);
    out[3] = palette.resolve(bottomRight.role, bottomRight.color);
}

} // namespace uilo
