$input v_color0, v_worldpos

#include <bgfx_shader.sh>

// u_clipRect / u_clipRect2:   xy = center (world px), zw = half size (world px)
// u_clipParams / u_clipParams2: x = radius (world px), y = enabled (>0.5 to apply)
// Two simultaneous rounded-rect SDF clips: the inner one is the shape
// being drawn (e.g. a Circle/RoundedRect own mask); the outer one is
// the most recent rounded ancestor (e.g. a parent panel). The final
// alpha is the product of both masks, so a child stays its own shape
// AND gets cropped by the parent's rounded corners.
uniform vec4 u_clipRect;
uniform vec4 u_clipParams;
uniform vec4 u_clipRect2;
uniform vec4 u_clipParams2;

float uiloRoundedAlpha(vec2 p, vec4 rect, vec4 params) {
    if (params.y < 0.5) return 1.0;
    vec2  c  = rect.xy;
    vec2  b  = rect.zw;
    float r  = params.x;
    vec2  q  = abs(p - c) - b + vec2_splat(r);
    float d  = length(max(q, vec2_splat(0.0))) +
               min(max(q.x, q.y), 0.0) - r;
    float aa = fwidth(d) + 1e-5;
    return 1.0 - smoothstep(-aa, aa, d);
}

void main() {
    vec4 c = v_color0;
    c.a *= uiloRoundedAlpha(v_worldpos, u_clipRect,  u_clipParams);
    c.a *= uiloRoundedAlpha(v_worldpos, u_clipRect2, u_clipParams2);
    if (c.a <= 0.0) discard;
    gl_FragColor = c;
}
