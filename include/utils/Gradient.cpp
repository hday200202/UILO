#include "Gradient.hpp"
#include "../Palette.hpp"

namespace uilo {

Gradient Gradient::vertical(GradientStop top, GradientStop bottom) {
    return Gradient(top, top, bottom, bottom);
}

Gradient Gradient::horizontal(GradientStop left, GradientStop right) {
    return Gradient(left, right, left, right);
}

void Gradient::resolve(const Palette& palette, Color out[4]) const {
    out[0] = palette.resolve(topLeft.role,     topLeft.color);
    out[1] = palette.resolve(topRight.role,    topRight.color);
    out[2] = palette.resolve(bottomLeft.role,  bottomLeft.color);
    out[3] = palette.resolve(bottomRight.role, bottomRight.color);
}

} // namespace uilo
