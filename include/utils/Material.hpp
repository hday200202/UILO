#pragma once

#include "Color.hpp"
#include <cstdint>

namespace uilo {

/*
    Material — describes a "look" effect applied to an element's background.

    Materials are carried on a Modifier (`Modifier::setMaterial`). Containers
    (Row/Column) and interactible widgets (Button, Dropdown) honour the
    material; everything else ignores it.

    API
    ---
    Each kind is exposed as a *static factory* that returns a Material by
    value, and Material itself has chainable setters that return `Material&`.
    This lets you tweak any preset inline at the call site, the same way
    `Modifier` and `*Options` work elsewhere in UILO:

        // Pick a preset and override two fields:
        row(
            Modifier()
                .setMaterial(
                    Material::Blur()
                        .setOpacity(0.4f)
                        .setBlurRadius(12.f)),
            ...);

        // Start from scratch (kind = None, no effect):
        auto m = Material()
                     .setKind(Material::Kind::Glass)
                     .setTint({200, 220, 255, 40})
                     .setOpacity(0.6f);

    Available kinds
    ---------------
      - None         no effect
      - Glass        Apple-style frosted glass: backdrop blur + tint + lens
      - Frosted      denser, more matte variant of Glass
      - Holographic  iridescent shifting tint (animated)
      - Liquid       sinusoidal UV ripples (animated)
      - Shimmer      Glass + diagonal highlight sweep (animated)
      - Aurora       soft drifting magenta/teal colour clouds (animated)
      - Tinted       glass that uses the element's own colour as tint
      - Ripple       Tinted + concentric ripples emanating from the cursor
      - Hover        Tinted + radial highlight under the cursor
      - Blur         semi-transparent blur of the backdrop, tinted with
                     the element's colour. Per-element blur radius.
*/

struct Material {
    enum class Kind : uint8_t {
        None        = 0,
        Glass       = 1,
        Frosted     = 2,
        Holographic = 3,
        Liquid      = 4,
        Shimmer     = 5,
        Aurora      = 6,
        Tinted      = 7,
        Ripple      = 8,
        Hover       = 9,
        Blur        = 10,
    };

    // ---------------- fields ------------------------------------------------
    Kind  kind          = Kind::None;
    // Tint applied on top of the blurred backdrop. Alpha = strength of tint.
    // For Tinted/Ripple/Hover/Blur the renderer replaces this with the
    // element's own colour automatically.
    Color tint          = Color{255, 255, 255, 32};
    // Saturation/brightness multipliers applied to the blurred backdrop.
    float saturation    = 1.0f;
    float brightness    = 1.02f;
    // Intensity of the 1px specular inner-rim ring (0..1+).
    float edgeHighlight = 1.0f;
    // Corner radius in pixels (rounded panels).
    float cornerRadius  = 14.f;
    // Body opacity (0..1). Lower = more of the underlying scene shows
    // through. Apple-style glass is around 0.55..0.75.
    float opacity       = 0.65f;
    // Edge refraction strength: how strongly the blurred backdrop is
    // pulled toward the centre near the rounded-rect boundary, in pixels.
    float refraction    = 28.f;
    // Animation tempo (cycles/sec-ish) and amplitude for *animated* kinds.
    float animSpeed     = 1.0f;
    float animStrength  = 1.0f;
    // Extra blur radius (in pixels) applied on top of the global blur
    // ladder. Only consumed by the Blur kind.
    float blurRadius    = 8.f;

    // ---------------- fluent setters ---------------------------------------
    Material& setKind          (Kind  k)        { kind          = k;     return *this; }
    Material& setTint          (Color c)        { tint          = c;     return *this; }
    Material& setSaturation    (float v)        { saturation    = v;     return *this; }
    Material& setBrightness    (float v)        { brightness    = v;     return *this; }
    Material& setEdgeHighlight (float v)        { edgeHighlight = v;     return *this; }
    Material& setCornerRadius  (float pxRadius) { cornerRadius  = pxRadius; return *this; }
    Material& setOpacity       (float v)        { opacity       = v;     return *this; }
    Material& setRefraction    (float pxAmt)    { refraction    = pxAmt; return *this; }
    Material& setAnimSpeed     (float v)        { animSpeed     = v;     return *this; }
    Material& setAnimStrength  (float v)        { animStrength  = v;     return *this; }
    Material& setBlurRadius    (float pxRadius) { blurRadius    = pxRadius; return *this; }

    // Convenience aliases — match the verbs in the user-facing docs.
    Material& setTransparency  (float v)        { opacity       = v;     return *this; }
    Material& setRadius        (float pxRadius) { blurRadius    = pxRadius; return *this; }

    // ---------------- presets (factories) ----------------------------------
    // Each returns a `Material` you can further customise with the setters
    // above. Defaults are tuned to look good out-of-the-box on a typical
    // UI background.

    static Material Glass() {
        return Material{}
            .setKind(Kind::Glass)
            .setTint({255, 255, 255, 32})
            .setSaturation(1.0f)
            .setBrightness(1.02f)
            .setEdgeHighlight(1.0f)
            .setCornerRadius(14.f)
            .setOpacity(0.65f)
            .setRefraction(28.f);
    }

    static Material Frosted() {
        return Material{}
            .setKind(Kind::Frosted)
            .setTint({255, 255, 255, 64})
            .setSaturation(0.55f)
            .setBrightness(1.05f)
            .setEdgeHighlight(0.85f)
            .setCornerRadius(18.f)
            .setOpacity(0.88f)
            .setRefraction(12.f);
    }

    static Material Holographic() {
        return Material{}
            .setKind(Kind::Holographic)
            .setTint({255, 255, 255, 90})
            .setSaturation(0.7f)
            .setBrightness(1.05f)
            .setCornerRadius(14.f)
            .setOpacity(0.75f)
            .setRefraction(18.f);
    }

    static Material Liquid() {
        return Material{}
            .setKind(Kind::Liquid)
            .setTint({180, 220, 255, 40})
            .setSaturation(1.05f)
            .setCornerRadius(16.f)
            .setOpacity(0.7f)
            .setRefraction(24.f);
    }

    static Material Shimmer() {
        return Material{}
            .setKind(Kind::Shimmer)
            .setTint({255, 255, 255, 32})
            .setCornerRadius(14.f)
            .setOpacity(0.7f)
            .setRefraction(28.f);
    }

    static Material Aurora() {
        return Material{}
            .setKind(Kind::Aurora)
            .setTint({120, 200, 255, 50})
            .setSaturation(0.85f)
            .setBrightness(1.05f)
            .setCornerRadius(18.f)
            .setOpacity(0.72f)
            .setRefraction(20.f);
    }

    // Tinted: glass body, but tinted with the element's own set color.
    static Material Tinted() {
        return Material{}
            .setKind(Kind::Tinted)
            .setSaturation(0.95f)
            .setCornerRadius(14.f)
            .setOpacity(0.85f)
            .setRefraction(22.f);
    }

    // Ripple: tinted glass + concentric waves from the cursor.
    static Material Ripple() {
        return Material{}
            .setKind(Kind::Ripple)
            .setSaturation(0.95f)
            .setCornerRadius(14.f)
            .setOpacity(0.88f)
            .setRefraction(22.f);
    }

    // Hover: tinted glass + soft radial highlight that follows the cursor.
    static Material Hover() {
        return Material{}
            .setKind(Kind::Hover)
            .setSaturation(0.95f)
            .setCornerRadius(14.f)
            .setOpacity(0.9f)
            .setRefraction(18.f);
    }

    // Blur: semi-transparent blur of what's behind the element, tinted
    // with the element's own colour. No lens/refraction or animation —
    // just a clean coloured frosted pane. Use `setBlurRadius(...)` (px)
    // to control how aggressive the blur is, and `setOpacity(...)`
    // (or `setTransparency(...)`) to control how strongly the colour
    // shows through.
    static Material Blur() {
        return Material{}
            .setKind(Kind::Blur)
            .setSaturation(1.0f)
            .setBrightness(1.0f)
            .setEdgeHighlight(0.6f)
            .setCornerRadius(14.f)
            .setOpacity(0.6f)
            .setRefraction(0.f)
            .setBlurRadius(8.f);
    }
};

} // namespace uilo
