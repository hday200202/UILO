#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../utils/Math.hpp"
#include "../utils/Color.hpp"
#include "../utils/Alignment.hpp"
#include "../utils/Material.hpp"
#include "Shapes.hpp"

// Forward-declare SDL and bgfx types so elements never need to include those
// headers directly. The Renderer owns all BGFX/SDL state.
struct SDL_Window;
namespace bgfx { struct FrameBufferHandle; }

namespace uilo {

// ---- Cursor types --------------------------------------------------------
enum class CursorType {
    Arrow,
    Hand,
    SizeHorizontal,
    SizeVertical,
    Text,
};

// ---- Opaque GPU resource handles -----------------------------------------
// Elements hold these but never touch bgfx directly.

struct Texture {
    uint16_t handle = UINT16_MAX;   // bgfx::TextureHandle.idx
    uint16_t width  = 0;
    uint16_t height = 0;
    bool valid() const { return handle != UINT16_MAX; }
};

struct Font {
    uint32_t id = UINT32_MAX;       // index into Renderer's font table
    bool valid() const { return id != UINT32_MAX; }
};

struct TextMetrics {
    Vec2f size      = {0.f, 0.f};   // total bounding box
    float ascent    = 0.f;          // pixels above baseline (positive)
    float descent   = 0.f;          // pixels below baseline (positive)
    float lineGap   = 0.f;          // recommended extra gap between lines
    float lineHeight() const { return ascent + descent + lineGap; }
};

// ---- Framebuffer handle (opaque wrapper around bgfx framebuffer) ---------
struct FrameBuffer {
    uint16_t handle  = UINT16_MAX;
    uint16_t viewId  = UINT16_MAX;
    Vec2u    size    = {0u, 0u};
    bool     valid() const { return handle != UINT16_MAX; }
};

// ---- Renderer ------------------------------------------------------------
class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // ---- Lifecycle --------------------------------------------------------
    bool init(uint32_t width, uint32_t height,
              const std::string& title = "UILO",
              uint8_t msaa = 4);
    void shutdown();

    void beginFrame();
    void endFrame();

    // ---- Window -----------------------------------------------------------
    Vec2u  getSize() const;
    void   setTitle(const std::string& title);
    void   setVsync(bool enabled);
    bool   getVsync() const;
    // Throttle the frame rate. Pass 0 (or any non-positive value) to disable.
    // With vsync also on, the effective rate is min(vsync, this limit).
    void   setFramerateLimit(float fps);
    float  getFramerateLimit() const;
    SDL_Window* sdlWindow() const { return m_window; }

    // ---- Cursor -----------------------------------------------------------
    void setCursor(CursorType type);

    // ---- Shape draw calls -------------------------------------------------
    void draw(const Rect&        rect);
    void draw(const RoundedRect& roundedRect);
    void draw(const Circle&      circle);
    void draw(const Triangle&    triangle);
    void draw(const Line&        line);

    // Batched line draw: emits all `count` lines in a single transient
    // vertex buffer / submit. Far cheaper than calling draw(Line) in a
    // loop when rendering many primitives (e.g. waveforms, grids).
    void drawLines(const Line* lines, size_t count);

    // Filled annular arc (gap-free triangle strip between innerR/outerR).
    // Angles in degrees, cartesian convention (0=+x, sweep increases CCW;
    // pass endDeg < startDeg for a clockwise sweep). Caller chooses
    // tessellation density via `segments` (clamped to >= 1).
    void drawArc(Vec2f center, float innerR, float outerR,
                 float startDeg, float endDeg, Color color, int segments);

    // ---- Texture / image --------------------------------------------------
    // Load an image file (png/jpg/etc.). Cached by path; safe to call
    // multiple times. Returns invalid Texture on failure.
    Texture loadTexture(const std::string& path);
    void    destroyTexture(Texture& tex);

    // Draw a textured quad in screen space. uv defaults to the whole image.
    // If `clipEllipse` is true, alpha is masked to the inscribed ellipse of
    // the destination rectangle.
    void drawImage(const Rectf& dst, const Texture& tex,
                   Color tint = Color::White,
                   Rectf uv   = {{0.f, 0.f}, {1.f, 1.f}},
                   bool flipH = false, bool flipV = false,
                   bool clipEllipse = false);

    // ---- Material effects -------------------------------------------------
    // Render an Apple-style "glass" panel of the given size at `dst`. Samples
    // the blurred backdrop captured in the previous frame's scene FB. Safe to
    // call from any element's render(). When the material is not Glass, this
    // is a no-op.
    //
    // `baseColor` is the element's own set color — Material kinds Tinted /
    // Ripple / Hover blend it into the panel so the original colour shows
    // through. Other kinds ignore it.
    void drawGlass(const Rectf& dst, const Material& mat,
                   Color baseColor = Color::Transparent);

    // ---- Mouse state for interactive materials ----------------------------
    // UILO calls this each frame after sampling the cursor. The renderer
    // uses it to drive Material::Ripple / Material::Hover (mouse trail
    // ripples and proximity glow). Coordinates are in framebuffer pixels.
    void setMouseState(Vec2f mousePosFbPx);

    // ---- Text -------------------------------------------------------------
    // Load a TTF font file. Cached by path. Returns invalid Font on failure.
    Font loadFont(const std::string& path);

    // Draw a UTF-8 string at `position` (top-left of the text box).
    // `sizePx` is the requested cap height in pixels.
    void drawText(const std::string& utf8,
                  Vec2f position,
                  const Font& font,
                  float sizePx,
                  Color color = Color::White);

    // Measure a UTF-8 string at the given size.
    TextMetrics measureText(const std::string& utf8,
                            const Font& font,
                            float sizePx);

    // Returns N+1 positions (relative to the top-left passed to drawText)
    // for each codepoint boundary in `utf8`. A '\n' advances the pen to
    // (0, prev_y + lineHeight). Position [N] is the trailing cursor slot.
    // Empty input yields a single {0,0} entry.
    std::vector<Vec2f> charPositions(const std::string& utf8,
                                     const Font& font,
                                     float sizePx);

    // ---- Framebuffer management -------------------------------------------
    FrameBuffer createFrameBuffer(Vec2u size);
    void        resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize);
    void        destroyFrameBuffer(FrameBuffer& fb);

    void pushFrameBuffer(FrameBuffer& fb);
    void popFrameBuffer();

    void drawFrameBuffer(const FrameBuffer& fb, Vec2f dest, Vec2f size,
                         Color tint = Color::White);

    void clear(Color color = Color::Transparent);

    // ---- Scissor clipping -------------------------------------------------
    void pushScissor(Rectf bounds);
    void popScissor();

    // ---- Rounded-rect clipping (SDF in fragment shaders) ------------------
    // Pushes a rounded clip region. All subsequent draws (solid/tex/text)
    // are alpha-masked to a rounded rect of the given bounds + radius.
    // Also pushes an axis-aligned scissor of `bounds` for early-out.
    // A radius <= 0 falls back to a plain rectangular scissor.
    void pushRoundClip(Rectf bounds, float radius);
    void popRoundClip();

    // ---- Rotation ---------------------------------------------------------
    // Degrees, standard cartesian convention: 0 = +x, 90 = +y,
    // 180 = -x, 270 = -y, 360 wraps to 0. Pivot is in screen-pixel coords.
    // Applied CPU-side to vertex positions of subsequent draws (Rect,
    // Circle, Triangle, Line, drawLines, drawImage, drawText). Note:
    // RoundedRect's SDF clip is axis-aligned and won't itself rotate; for
    // knob indicators prefer Image/Triangle/Line drawn on top of a Circle.
    void setRotation(float degrees, Vec2f pivot);
    void rotate(float deltaDegrees);
    void clearRotation();

private:
    SDL_Window* m_window      = nullptr;
    uint32_t    m_lastWidth   = 0;
    uint32_t    m_lastHeight  = 0;
    uint8_t     m_msaa        = 4;
    uint32_t    m_resetFlags  = 0;
    bool        m_initialised = false;
    double      m_frameInterval = 0.0; // seconds per frame; 0 = unlimited
    uint64_t    m_nextFrameTick = 0;   // steady_clock ns of next frame deadline

    // Mouse state plumbed through by UILO each frame so interactive
    // materials (Ripple / Hover) can sample a global cursor. Coordinates
    // are in framebuffer pixels.
    Vec2f       m_mousePos      = { -1.f, -1.f };
    Vec2f       m_mousePosPrev  = { -1.f, -1.f };
    float       m_mouseLastMoveT = 0.f; // value of impl.elapsed at last move

    // Views 0..3 are reserved for the scene FB + blur ladder + composite
    // (see RendererImpl.hpp::Impl). User framebuffers start at 4.
    uint16_t m_nextViewId = 4;

    struct ViewEntry { uint16_t viewId; };
    static constexpr int kMaxViewStack = 16;
    ViewEntry m_viewStack[kMaxViewStack] = {};
    int       m_viewStackTop = 0;

    uint16_t currentViewId() const;
    void     submitOrtho(uint16_t viewId, Vec2u size);

public:
    // PIMPL is public so TU-local helpers (in renderer .cpp files that include
    // RendererImpl.hpp) can reference Impl. The Impl struct itself is only
    // declared in the private RendererImpl.hpp header.
    struct Impl;
private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace uilo

