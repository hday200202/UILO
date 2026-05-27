$input v_color0, v_texcoord0, v_worldpos

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

// x = clipEllipse flag (1 = enabled, 0 = disabled)
uniform vec4 u_imgFlags;
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
    vec4 c = texture2D(s_texColor, v_texcoord0) * v_color0;

    if (u_imgFlags.x > 0.5) {
        vec2  d  = v_texcoord0 - vec2(0.5, 0.5);
        float r2 = dot(d, d) * 4.0;
        float aa = fwidth(r2) + 1e-5;
        c.a *= 1.0 - smoothstep(1.0 - aa, 1.0 + aa, r2);
    }

    c.a *= uiloClipAlpha(v_worldpos);
    if (c.a <= 0.0) discard;
    gl_FragColor = c;
}
