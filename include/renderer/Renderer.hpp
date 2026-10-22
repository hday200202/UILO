#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../utils/Math.hpp"
#include "../utils/Color.hpp"
#include "../utils/Alignment.hpp"
/* CursorType lives in its own header so Modifier can carry one without pulling
   in the renderer. */
#include "../utils/Cursor.hpp"
#include "Shapes.hpp"

/* Forward-declared so elements never need VALO's rendering headers. The
   Renderer owns the window and every GPU resource; VALO does the rest. */
namespace valo { class Window; }

namespace uilo {

/* Opaque GPU resource handles. */
/* Elements hold these but never touch VALO directly. */

struct Texture {
    uint16_t handle = UINT16_MAX;   /* VALO TextureID */
    uint16_t width  = 0;
    uint16_t height = 0;
    bool valid() const { return handle != UINT16_MAX; }
};

/*
    Font:
    - Desc:     An opaque handle to a loaded font, an index into the renderer's
                font table. Invalid until loadFont succeeds, which is what a
                caller checks before drawing rather than testing the path.
*/
struct Font {
    uint32_t id = UINT32_MAX;   /* index into Renderer's font table */
    bool valid() const { return id != UINT32_MAX; }
};

/*
    TextMetrics:
    - Desc:     Measurements of a laid-out string: the bounding box plus the
                vertical metrics of the face it was measured with. lineHeight()
                is ascent plus descent plus the recommended gap, which is the
                spacing text should be laid out on.
*/
struct TextMetrics {
    Vec2f size      = {0.f, 0.f};   /* total bounding box */
    float ascent    = 0.f;   /* pixels above baseline (positive) */
    float descent   = 0.f;   /* pixels below baseline (positive) */
    float lineGap   = 0.f;   /* recommended extra gap between lines */
    float lineHeight() const { return ascent + descent + lineGap; }
};

/* Synthetic text styling, applied when the glyph quads are emitted rather than
   by loading a second font file -- so any face can be emboldened or slanted. */
struct TextStyle {
    bool bold   = false;
    bool italic = false;

    bool none() const { return !bold && !italic; }
};

/* Renderer counters for the previous frame. Useful for HUD overlays and perf
   instrumentation. */
struct RendererStats {
    uint32_t numDraw     = 0;
    uint32_t numVertices = 0;
    double   cpuTimeMs   = 0.0;
    double   gpuTimeMs   = 0.0;
};

/* Framebuffer handle (opaque wrapper around a VALO render target) */
struct FrameBuffer {
    uint16_t handle  = UINT16_MAX;   /* VALO RenderTargetID */
    uint16_t viewId  = UINT16_MAX;   /* VALO layer the framebuffer draws through */
    Vec2u    size    = {0u, 0u};
    bool     valid() const { return handle != UINT16_MAX; }
};

/* Renderer */
class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    /* Lifecycle */
    /* Opens the window and brings VALO up on it. The backend is Vulkan where
       it is available and the software rasteriser otherwise; UILO_RENDERER=
       vulkan | software in the environment forces one. */
    bool init(uint32_t width, uint32_t height,
              const std::string& title = "UILO",
              uint8_t msaa = 4);
    bool ownsContext() const { return true; }
    void shutdown();

    void beginFrame();
    void endFrame();

    /* Window */
    Vec2u  getSize() const;
    void   setTitle(const std::string& title);
    void   setVsync(bool enabled);
    bool   getVsync() const;
    /* Throttle the frame rate. Pass 0 (or any non-positive value) to disable.
       With vsync also on, the effective rate is min(vsync, this limit). */
    void   setFramerateLimit(float fps);
    float  getFramerateLimit() const;
    /* The window UILO reads its input from. Null before init(). */
    valo::Window* window() const;

    /* Counters for the most recently submitted frame. Cheap; safe to call once
       per frame. */
    RendererStats getStats() const;

    /* Cursor */
    void setCursor(CursorType type);

    /* Shape draw calls */
    void draw(const Rect&        rect);
    void draw(const RoundedRect& roundedRect);
    void draw(const Circle&      circle);
    void draw(const Triangle&    triangle);
    void draw(const Line&        line);

    /* Batched line draw: emits all `count` lines in a single draw. */
    void drawLines(const Line* lines, size_t count);

    /* Filled annular arc (gap-free triangle strip between innerR/outerR). */
    void drawArc(Vec2f center, float innerR, float outerR,
                 float startDeg, float endDeg, Color color, int segments);

    /* Texture / image. */
    /* Load an image file (png/jpg/etc.). */
    Texture loadTexture(const std::string& path);
    void    destroyTexture(Texture& tex);

    /* Decode an image file to tightly-packed RGBA8 bytes without creating a
       GPU texture (row-major, top-left origin). */
    bool loadImagePixels(const std::string& path, std::vector<uint8_t>& outRgba,
                         uint32_t& outWidth, uint32_t& outHeight);

    /* Create an empty *mutable* RGBA8 texture. */
    Texture createTexture(uint16_t width, uint16_t height);

    /* Replace the full contents of a texture made by createTexture with
       width*height*4 RGBA8 bytes. No-op on invalid texture / null pixels. */
    void updateTexture(const Texture& tex, const uint8_t* rgba);

    /* Draw a textured quad in screen space. */
    void drawImage(const Rectf& dst, const Texture& tex,
                   Color tint = Color::White,
                   Rectf uv   = {{0.f, 0.f}, {1.f, 1.f}},
                   bool flipH = false, bool flipV = false,
                   bool clipEllipse = false);

    /* Text. */
    /* Load a TTF font file. Cached by path. Returns invalid Font on failure. */
    Font loadFont(const std::string& path);

    /* Draw a UTF-8 string at `position` (top-left of the text box). `sizePx`
       is the requested cap height in pixels. */
    void drawText(const std::string& utf8,
                  Vec2f position,
                  const Font& font,
                  float sizePx,
                  Color color = Color::White,
                  TextStyle style = {});

    /* Measure a UTF-8 string at the given size. */
    TextMetrics measureText(const std::string& utf8,
                            const Font& font,
                            float sizePx);

    /* Returns N+1 positions (relative to the top-left passed to drawText) for
       each codepoint boundary in `utf8`. */
    std::vector<Vec2f> charPositions(const std::string& utf8,
                                     const Font& font,
                                     float sizePx);

    /* Framebuffer management */
    FrameBuffer createFrameBuffer(Vec2u size);
    void        resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize);
    void        destroyFrameBuffer(FrameBuffer& fb);

    void pushFrameBuffer(FrameBuffer& fb);
    void popFrameBuffer();

    void drawFrameBuffer(const FrameBuffer& fb, Vec2f dest, Vec2f size,
                         Color tint = Color::White);

    void clear(Color color = Color::Transparent);

    /* Scissor clipping */
    void pushScissor(Rectf bounds);
    void popScissor();

    /* Rounded-rect clipping (SDF in fragment shaders). */
    /* Pushes a rounded clip region. */
    void pushRoundClip(Rectf bounds, float radius);
    void popRoundClip();

    /* Rotation. */
    /* Degrees, standard cartesian convention: 0 = +x, 90 = +y, 180 = -x, 270
       =. */
    void setRotation(float degrees, Vec2f pivot);
    void rotate(float deltaDegrees);
    void clearRotation();

private:
    uint32_t    m_lastWidth   = 0;
    uint32_t    m_lastHeight  = 0;
    uint8_t     m_msaa        = 4;
    bool        m_initialised = false;
    double      m_frameInterval = 0.0;   /* seconds per frame; 0 = unlimited */
    uint64_t    m_nextFrameTick = 0;   /* steady_clock ns of next frame deadline */

public:
    /* PIMPL is public so TU-local helpers (in renderer .cpp files that include
       RendererImpl.hpp) can reference Impl. */
    struct Impl;
private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace uilo
