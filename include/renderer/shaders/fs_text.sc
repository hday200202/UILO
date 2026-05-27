$input v_color0, v_texcoord0, v_worldpos

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

uniform vec4 u_clipRect;
uniform vec4 u_clipParams;

float uiloClipAlpha(vec2 p) {
    if (u_clipParams.y < 0.5) return 1.0;
    vec2  c  = u_clipRect.xy;
    vec2  b  = u_clipRect.zw;
    float r  = u_clipParams.x;
    vec2  q  = abs(p - c) - b + vec2_splat(r);
    float d  = length(max(q, vec2_splat(0.0))) +
               min(max(q.x, q.y), 0.0) - r;
    float aa = fwidth(d) + 1e-5;
    return 1.0 - smoothstep(-aa, aa, d);
}

void main() {
    float a = texture2D(s_texColor, v_texcoord0).x;
    float ca = v_color0.a * a * uiloClipAlpha(v_worldpos);
    if (ca <= 0.0) discard;
    gl_FragColor = vec4(v_color0.rgb, ca);
}
