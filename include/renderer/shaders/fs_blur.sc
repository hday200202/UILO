$input v_color0, v_texcoord0, v_worldpos

#include <bgfx_shader.sh>

// Separable Gaussian blur — invoked twice per frame: once horizontally,
// once vertically. u_blurParams.xy = per-tap offset in normalised UV space
// (already pre-multiplied by direction + scale on the CPU side).
SAMPLER2D(s_texColor, 0);
uniform vec4 u_blurParams; // xy = uv step, z = unused, w = unused

void main() {
    vec2 step = u_blurParams.xy;
    vec4 c = vec4_splat(0.0);
    // 9-tap Gaussian (sigma ~= 2). Weights sum to 1.0.
    c += texture2D(s_texColor, v_texcoord0 - step * 4.0) * 0.0162162162;
    c += texture2D(s_texColor, v_texcoord0 - step * 3.0) * 0.0540540541;
    c += texture2D(s_texColor, v_texcoord0 - step * 2.0) * 0.1216216216;
    c += texture2D(s_texColor, v_texcoord0 - step * 1.0) * 0.1945945946;
    c += texture2D(s_texColor, v_texcoord0                ) * 0.2270270270;
    c += texture2D(s_texColor, v_texcoord0 + step * 1.0) * 0.1945945946;
    c += texture2D(s_texColor, v_texcoord0 + step * 2.0) * 0.1216216216;
    c += texture2D(s_texColor, v_texcoord0 + step * 3.0) * 0.0540540541;
    c += texture2D(s_texColor, v_texcoord0 + step * 4.0) * 0.0162162162;
    gl_FragColor = c;
}
