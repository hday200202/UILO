$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

// x = clipEllipse flag (1 = enabled, 0 = disabled)
uniform vec4 u_imgFlags;

void main() {
    vec4 c = texture2D(s_texColor, v_texcoord0) * v_color0;

    if (u_imgFlags.x > 0.5) {
        // Inscribed ellipse: alpha = 1 inside, smooth edge using screen-space
        // derivatives so the falloff is roughly one pixel wide regardless of
        // the quad's size.
        vec2  d  = v_texcoord0 - vec2(0.5, 0.5);
        float r2 = dot(d, d) * 4.0;            // 1.0 at the ellipse boundary
        float aa = fwidth(r2) + 1e-5;
        c.a *= 1.0 - smoothstep(1.0 - aa, 1.0 + aa, r2);
    }

    gl_FragColor = c;
}
