$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

// "Liquid glass" composite — samples a pre-blurred backdrop and overlays a
// tint plus per-kind effects (refraction, iridescence, ripples, shimmer,
// aurora). All material kinds share this single shader and branch on
// u_glassAnim.x (the Material::Kind enum cast to float).
//
//   s_texColor      : blurred scene (full-screen texture sampled at the
//                     element's screen UV — passed in via v_texcoord0).
//   u_glassParams   : x = body opacity (0..1) — final glass alpha multiplier
//                     y = saturation multiplier on the backdrop
//                     z = brightness multiplier on the backdrop
//                     w = edge-highlight intensity
//   u_glassTint     : rgba; alpha = tint strength (mixed straight in).
//   u_glassRect     : x = corner radius in pixels
//                     y = half-width  of element in pixels
//                     z = half-height of element in pixels
//                     w = refraction strength in UV units (per-axis)
//   u_glassAnim     : x = kind (0=None,1=Glass,2=Frosted,3=Holographic,
//                               4=Liquid,5=Shimmer,6=Aurora)
//                     y = elapsed time in seconds
//                     z = anim speed multiplier
//                     w = anim strength multiplier
//
// v_color0 carries the per-vertex (a,b) local 0..1 coords in the .rg
// channels (packed by the CPU side; .ba unused). We use these for the
// rounded-rect SDF so the shader doesn't need to know screen-space
// orientation.

SAMPLER2D(s_texColor, 0);
uniform vec4 u_glassParams;
uniform vec4 u_glassTint;
uniform vec4 u_glassRect;
uniform vec4 u_glassAnim;
uniform vec4 u_glassBase;   // rgba: element's own colour (Tinted/Ripple/Hover)
uniform vec4 u_glassMouse;  // xy = cursor in element-local 0..1
                            // z  = 1 if cursor inside this rect, else 0
                            // w  = seconds since the cursor last moved

// Signed distance to a rounded box centred at the origin.
float sdRoundBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2_splat(r);
    return length(max(q, vec2_splat(0.0))) +
           min(max(q.x, q.y), 0.0) - r;
}

// Smooth cloud-like value at (p, t) for the Aurora effect. Combines a few
// sinusoids so it reads as soft drifting clouds without a noise texture.
float cloud(vec2 p, float t) {
    float a = sin(p.x * 1.3 + t * 0.6) +
              sin(p.y * 1.1 - t * 0.4) +
              sin((p.x + p.y) * 0.8 + t * 0.5);
    float b = sin(p.x * 0.5 - t * 0.3) *
              cos(p.y * 0.7 + t * 0.5);
    return (a + b) * 0.18 + 0.5;
}

void main() {
    // ---- rounded-rect SDF in element-local pixel space ------------------
    vec2 halfSize = u_glassRect.yz;
    vec2 p        = (v_color0.rg - vec2_splat(0.5)) * (halfSize * 2.0);
    float radius  = u_glassRect.x;
    float d       = sdRoundBox(p, halfSize, radius);

    // ---- per-kind unpack ------------------------------------------------
    float kind   = u_glassAnim.x;
    float time   = u_glassAnim.y * u_glassAnim.z;   // already-scaled time
    float aStr   = u_glassAnim.w;

    // ---- refraction: lens-like inward bend near the boundary -----------
    float bendWidth = max(radius * 2.0, 32.0);
    float bend      = smoothstep(-bendWidth, 0.0, d);
    bend            = bend * bend * (3.0 - 2.0 * bend);
    vec2  nrm       = normalize(p + vec2(0.0001, 0.0001));
    vec2  refractUv = v_texcoord0 - nrm * bend * u_glassRect.w;

    // Liquid kind: add full-body sinusoidal ripples on top of the lens.
    if (kind > 3.5 && kind < 4.5) {
        vec2 ripple = vec2(
            sin(p.y * 0.035 + time * 1.6),
            cos(p.x * 0.035 + time * 1.3)
        ) * (0.004 * aStr);
        refractUv += ripple;
    }

    // Ripple kind: refract along the radial direction from the cursor, so
    // the concentric waves visibly distort the backdrop (not just the
    // tint). Only active when the cursor is over the element.
    if (kind > 7.5 && kind < 8.5 && u_glassMouse.z > 0.5) {
        // Cursor in element-local pixel space (matches `p`).
        vec2 mPx     = (u_glassMouse.xy - vec2_splat(0.5)) * (halfSize * 2.0);
        vec2 toMouse = p - mPx;
        float distPx = length(toMouse) + 0.001;
        vec2 dir     = toMouse / distPx;
        float fade   = exp(-u_glassMouse.w * 1.8);             // calm down on idle
        float wave   = sin(distPx * 0.08 - time * 6.0) * fade;
        // Convert pixel offset (~3 px) to UV units of the blur target.
        // u_glassRect.w is already refraction-in-UV per *pixel of bend*;
        // we just borrow the same conversion via 1/(min screen extent).
        refractUv += dir * wave * (u_glassRect.w * 0.18);
    }

    // ---- sample blurred backdrop ----------------------------------------
    // For most kinds a single tap of the pre-blurred ladder is enough.
    // The Blur kind takes a small 9-tap box-blur on top of that for an
    // additional, per-element-configurable softness (u_glassAnim.w is
    // repurposed as a pixel radius in this branch).
    vec3 bg;
    if (kind > 9.5 && kind < 10.5) {
        // Convert radius from screen pixels to UV units of the blur target.
        // u_glassRect.w packs (refraction in UV / px); for the Blur kind
        // refraction is 0 so we recover the conversion factor from the
        // smaller window axis instead — encoded as the inverse of the
        // bend width fallback above (32 px → ~1/min(W,H)).
        float radiusPx = u_glassAnim.w;
        // Approximate UV-per-pixel using the local element extent: the
        // blur target spans the whole window, so 1 UV unit ≈ window size.
        // We use the element half-size as a proxy: element occupies
        // (halfSize*2 / windowSize) UV. Reverse-engineer via the known
        // local-pixel-to-UV ratio from refractUv math is tricky — instead
        // we just use a heuristic px-to-UV = 1/2048 (works on typical
        // desktop windows, scales linearly with radiusPx anyway).
        float ux = radiusPx * (1.0 / 2048.0);
        vec3 acc = vec3_splat(0.0);
        // 9 taps in a plus/X pattern. Cheap and visually adequate.
        acc += texture2D(s_texColor, refractUv).rgb                     * 0.20;
        acc += texture2D(s_texColor, refractUv + vec2( ux,  0.0)).rgb   * 0.12;
        acc += texture2D(s_texColor, refractUv + vec2(-ux,  0.0)).rgb   * 0.12;
        acc += texture2D(s_texColor, refractUv + vec2( 0.0,  ux)).rgb   * 0.12;
        acc += texture2D(s_texColor, refractUv + vec2( 0.0, -ux)).rgb   * 0.12;
        acc += texture2D(s_texColor, refractUv + vec2( ux,  ux)).rgb    * 0.08;
        acc += texture2D(s_texColor, refractUv + vec2(-ux,  ux)).rgb    * 0.08;
        acc += texture2D(s_texColor, refractUv + vec2( ux, -ux)).rgb    * 0.08;
        acc += texture2D(s_texColor, refractUv + vec2(-ux, -ux)).rgb    * 0.08;
        bg = acc;
    } else {
        bg = texture2D(s_texColor, refractUv).rgb;
    }

    // saturation lift (toward / away from luminance)
    float l = dot(bg, vec3(0.2126, 0.7152, 0.0722));
    bg = mix(vec3_splat(l), bg, u_glassParams.y);
    // brightness lift
    bg *= u_glassParams.z;

    // ---- tint over backdrop --------------------------------------------
    vec3 tintRGB   = u_glassTint.rgb;
    float tintMix  = clamp(u_glassTint.a, 0.0, 1.0);

    // Tinted / Ripple / Hover: replace the static white tint with the
    // element's own colour so the original "set color" of the panel still
    // shows through. Alpha of the base colour scales how strong the
    // overlay is, so a transparent base still gets bare glass.
    if (kind > 6.5 && kind < 9.5 && u_glassBase.a > 0.001) {
        tintRGB = u_glassBase.rgb;
        // Use the base alpha as the overlay strength; bias up a touch so
        // a fully-opaque source colour reads as a solid coloured panel.
        tintMix = clamp(u_glassBase.a * 0.95, 0.0, 0.95);
    }

    // Blur kind: same colour-takeover, but the *mix* of colour vs blurred
    // backdrop is driven by the user-facing opacity/transparency knob
    // rather than the base alpha. Higher opacity → colour dominates;
    // lower → the blur shows through. Final element alpha is forced to
    // 1.0 below so "transparency" reads as "see-through to the blur",
    // not "see-through to the *unblurred* scene".
    if (kind > 9.5 && kind < 10.5 && u_glassBase.a > 0.001) {
        tintRGB = u_glassBase.rgb;
        tintMix = clamp(u_glassParams.x, 0.0, 1.0);
    }

    // Holographic: replace static tint with a position+time iridescence.
    if (kind > 2.5 && kind < 3.5) {
        float ph = (v_color0.r + v_color0.g) * 6.2831 + time * 1.6;
        vec3 iri = vec3(
            0.5 + 0.5 * sin(ph),
            0.5 + 0.5 * sin(ph + 2.094),
            0.5 + 0.5 * sin(ph + 4.188)
        );
        tintRGB = mix(tintRGB, iri, 0.85 * aStr);
        tintMix = clamp(tintMix * 1.4, 0.0, 0.9);
    }

    // Aurora: animated soft cloud overlay.
    if (kind > 5.5 && kind < 6.5) {
        vec2 cp = p * 0.04;
        float c1 = cloud(cp,                    time * 0.9);
        float c2 = cloud(cp + vec2(3.7, 1.2),   time * 0.7 + 1.3);
        vec3 auroraCol = mix(
            vec3(0.18, 0.55, 0.95),
            vec3(0.82, 0.30, 0.95),
            c1
        );
        auroraCol = mix(auroraCol, vec3(0.30, 0.95, 0.55), c2 * 0.5);
        tintRGB = auroraCol;
        tintMix = clamp(0.55 * aStr + tintMix * 0.4, 0.0, 0.95);
    }

    vec3 col = mix(bg, tintRGB, tintMix);

    // Ripple kind: also paint a luminance ring on top of the surface so
    // the waves are visible even on dark backdrops. Centred at the cursor.
    if (kind > 7.5 && kind < 8.5) {
        vec2 mPx     = (u_glassMouse.xy - vec2_splat(0.5)) * (halfSize * 2.0);
        float distPx = length(p - mPx);
        float fade   = exp(-u_glassMouse.w * 1.8) * u_glassMouse.z;
        // Cosine ring with radial falloff (~200 px reach).
        float ring   = cos(distPx * 0.08 - time * 6.0);
        ring         = ring * exp(-distPx * 0.012);
        col         += vec3_splat(0.35 * aStr) * ring * fade;
    }

    // Hover kind: soft radial highlight that follows the cursor while it
    // is inside the element. Fades as the cursor leaves.
    if (kind > 8.5 && kind < 9.5) {
        vec2 mPx     = (u_glassMouse.xy - vec2_splat(0.5)) * (halfSize * 2.0);
        float distPx = length(p - mPx);
        // Halo radius scales with the smaller axis so it reads on both
        // narrow buttons and tall panels.
        float reach  = max(min(halfSize.x, halfSize.y), 32.0) * 1.6;
        float halo   = 1.0 - smoothstep(0.0, reach, distPx);
        halo         = halo * halo;                            // gamma-curve
        // Fade out smoothly when the cursor leaves the rect.
        float gate   = u_glassMouse.z;
        col += vec3_splat(0.28 * aStr) * halo * gate;
        // A subtle brightening of the base tint reinforces the affordance.
        col = mix(col, col * 1.08, halo * gate);
    }

    // Shimmer: diagonal highlight sweep across the surface.
    if (kind > 4.5 && kind < 5.5) {
        float diag  = (v_color0.r + v_color0.g) * 0.5;
        float sweep = fract(time * 0.33);
        float band  = 1.0 - smoothstep(0.0, 0.08, abs(diag - sweep));
        col        += vec3_splat(0.35 * aStr) * band;
    }

    // ---- coverage mask --------------------------------------------------
    float aaPx     = 1.0;
    float fillMask = 1.0 - smoothstep(0.0, aaPx, d);

    // ---- Apple-style inner rim lighting (shared by all kinds) ----------
    // The Blur kind opts out: the user just wants a plain coloured pane
    // over a blurred backdrop, no glass / rim / specular cues.
    if (kind < 9.5 || kind > 10.5) {
        float rim     = smoothstep(-1.5, 0.0, d) * (1.0 - smoothstep(0.0, aaPx, d));
        float topness = clamp(-p.y / max(halfSize.y, 1.0), 0.0, 1.0);
        float botness = clamp( p.y / max(halfSize.y, 1.0), 0.0, 1.0);

        col += vec3_splat(0.18) * rim * topness * u_glassParams.w;
        col -= vec3_splat(0.10) * rim * botness * u_glassParams.w;

        float topGrad = 1.0 - smoothstep(-halfSize.y, -halfSize.y * 0.4, p.y);
        col += vec3_splat(0.04) * topGrad * u_glassParams.w;
    }

    // Final alpha: body opacity scaled by coverage. The rim is a luminance
    // change (no alpha pop) so panels blend smoothly into the bg.
    // Final alpha: body opacity scaled by coverage. The Blur kind already
    // consumed `opacity` as the colour/blur mix above, so for that kind
    // we keep the panel fully opaque (the perceived transparency comes
    // from the blurred backdrop bleeding through `col`).
    float bodyOpacity = u_glassParams.x;
    if (kind > 9.5 && kind < 10.5) bodyOpacity = 1.0;
    float alpha = fillMask * bodyOpacity;

    gl_FragColor = vec4(col, alpha);
}
