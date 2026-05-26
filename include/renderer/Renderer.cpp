#include "RendererImpl.hpp"

#include <SDL3/SDL.h>

// Disable embedded-shader references to backends we don't compile for.
// Must come before <bgfx/embedded_shader.h>.
#if !defined(BX_PLATFORM_WINDOWS) || !BX_PLATFORM_WINDOWS
#  define BGFX_PLATFORM_SUPPORTS_DXBC 0
#  define BGFX_PLATFORM_SUPPORTS_DXIL 0
#endif
#define BGFX_PLATFORM_SUPPORTS_WGSL 0
#define BGFX_PLATFORM_SUPPORTS_PSSL 0
#define BGFX_PLATFORM_SUPPORTS_NVN  0

#include <bgfx/embedded_shader.h>
#include <cassert>

#if defined(SDL_PLATFORM_LINUX)
#  include <X11/Xlib.h>
#endif

// ============================================================================
//  Embedded shaders (compiled per-backend by bgfx-shaderc at build time)
// ============================================================================

#include "spirv/vs_solid.sc.bin.h"
#include "spirv/vs_tex.sc.bin.h"
#include "spirv/fs_solid.sc.bin.h"
#include "spirv/fs_tex.sc.bin.h"
#include "spirv/fs_text.sc.bin.h"

#include "glsl/vs_solid.sc.bin.h"
#include "glsl/vs_tex.sc.bin.h"
#include "glsl/fs_solid.sc.bin.h"
#include "glsl/fs_tex.sc.bin.h"
#include "glsl/fs_text.sc.bin.h"

#include "essl/vs_solid.sc.bin.h"
#include "essl/vs_tex.sc.bin.h"
#include "essl/fs_solid.sc.bin.h"
#include "essl/fs_tex.sc.bin.h"
#include "essl/fs_text.sc.bin.h"

#if BX_PLATFORM_OSX || BX_PLATFORM_IOS
#  include "metal/vs_solid.sc.bin.h"
#  include "metal/vs_tex.sc.bin.h"
#  include "metal/fs_solid.sc.bin.h"
#  include "metal/fs_tex.sc.bin.h"
#  include "metal/fs_text.sc.bin.h"
#endif

#if BX_PLATFORM_WINDOWS
#  include "dx11/vs_solid.sc.bin.h"
#  include "dx11/vs_tex.sc.bin.h"
#  include "dx11/fs_solid.sc.bin.h"
#  include "dx11/fs_tex.sc.bin.h"
#  include "dx11/fs_text.sc.bin.h"
#endif

namespace uilo {

namespace {
    static const bgfx::EmbeddedShader s_embeddedShaders[] = {
        BGFX_EMBEDDED_SHADER(vs_solid),
        BGFX_EMBEDDED_SHADER(fs_solid),
        BGFX_EMBEDDED_SHADER(vs_tex),
        BGFX_EMBEDDED_SHADER(fs_tex),
        BGFX_EMBEDDED_SHADER(fs_text),
        BGFX_EMBEDDED_SHADER_END(),
    };
}

// ============================================================================
//  Impl
// ============================================================================

void Renderer::Impl::ensureLayouts() {
    if (layoutsInit) return;
    solidLayout.begin()
        .add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
        .end();
    texLayout.begin()
        .add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    layoutsInit = true;
}

bool Renderer::Impl::initShaders() {
    const bgfx::RendererType::Enum type = bgfx::getRendererType();

    bgfx::ShaderHandle vs   = bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_solid");
    bgfx::ShaderHandle fs   = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_solid");
    bgfx::ShaderHandle vst1 = bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_tex");
    bgfx::ShaderHandle vst2 = bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_tex");
    bgfx::ShaderHandle fst  = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_tex");
    bgfx::ShaderHandle ftx  = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_text");
    if (!bgfx::isValid(vs)  || !bgfx::isValid(fs)  ||
        !bgfx::isValid(vst1)|| !bgfx::isValid(vst2)||
        !bgfx::isValid(fst) || !bgfx::isValid(ftx)) {
        std::fprintf(stderr, "[UILO] Failed to create shaders (renderer=%s)\n",
                     bgfx::getRendererName(type));
        return false;
    }
    solidProgram = bgfx::createProgram(vs,   fs,  true);
    texProgram   = bgfx::createProgram(vst1, fst, true);
    textProgram  = bgfx::createProgram(vst2, ftx, true);
    s_texColor   = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
    if (!bgfx::isValid(solidProgram) ||
        !bgfx::isValid(texProgram)   ||
        !bgfx::isValid(textProgram)) {
        std::fprintf(stderr, "[UILO] Failed to create shader programs\n");
        return false;
    }
    return true;
}
void Renderer::Impl::shutdownResources() {
    for (auto& kv : textureCache) {
        if (kv.second.handle != UINT16_MAX) {
            bgfx::TextureHandle h{ kv.second.handle };
            bgfx::destroy(h);
        }
    }
    textureCache.clear();

    for (auto& rec : fonts) {
        for (auto& kv : rec.sizes) {
            if (bgfx::isValid(kv.second.atlas))
                bgfx::destroy(kv.second.atlas);
        }
    }
    fonts.clear();
    fontByPath.clear();

    for (auto& kv : cursors) {
        if (kv.second) SDL_DestroyCursor((SDL_Cursor*)kv.second);
    }
    cursors.clear();

    if (bgfx::isValid(s_texColor))   bgfx::destroy(s_texColor);
    if (bgfx::isValid(solidProgram)) bgfx::destroy(solidProgram);
    if (bgfx::isValid(texProgram))   bgfx::destroy(texProgram);
    if (bgfx::isValid(textProgram))  bgfx::destroy(textProgram);
    s_texColor   = BGFX_INVALID_HANDLE;
    solidProgram = BGFX_INVALID_HANDLE;
    texProgram   = BGFX_INVALID_HANDLE;
    textProgram  = BGFX_INVALID_HANDLE;
}

// ============================================================================
//  Renderer ctor/dtor/lifecycle
// ============================================================================

Renderer::Renderer() : m_impl(std::make_unique<Impl>()) {}
Renderer::~Renderer() { shutdown(); }

bool Renderer::init(uint32_t width, uint32_t height,
                    const std::string& title, uint8_t msaa) {
    if (m_initialised) return true;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "[UILO] SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    uint32_t flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    m_window = SDL_CreateWindow(title.c_str(), (int)width, (int)height, flags);
    if (!m_window) {
        std::fprintf(stderr, "[UILO] SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    // Use real backing-pixel size (HiDPI displays make this larger than the
    // logical width/height requested above).
    {
        int pxW = (int)width, pxH = (int)height;
        SDL_GetWindowSizeInPixels(m_window, &pxW, &pxH);
        width  = (uint32_t)pxW;
        height = (uint32_t)pxH;
    }

    bgfx::PlatformData pd{};
#if defined(SDL_PLATFORM_WIN32)
    SDL_PropertiesID props = SDL_GetWindowProperties(m_window);
    pd.nwh = SDL_GetPointerProperty(props,
                SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(SDL_PLATFORM_MACOS)
    pd.nwh = SDL_Metal_CreateView(m_window);
#elif defined(SDL_PLATFORM_LINUX)
    SDL_PropertiesID props = SDL_GetWindowProperties(m_window);
    void* waylandSurf = SDL_GetPointerProperty(props,
                SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    Window x11WinNum  = (Window)SDL_GetNumberProperty(props,
                SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    void* x11Win      = (void*)(uintptr_t)x11WinNum;
    void* x11Display  = SDL_GetPointerProperty(props,
                SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    if (waylandSurf) {
        pd.ndt  = SDL_GetPointerProperty(props,
                     SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        pd.nwh  = waylandSurf;
        pd.type = bgfx::NativeWindowHandleType::Wayland;
    } else {
        pd.ndt = x11Display;
        pd.nwh = x11Win;
    }
#endif

    bgfx::Init init;
    init.platformData      = pd;
    init.resolution.width  = width;
    init.resolution.height = height;
    if (pd.type == bgfx::NativeWindowHandleType::Wayland)
        init.type = bgfx::RendererType::Vulkan;

    uint32_t resetFlags = BGFX_RESET_VSYNC;
    if      (msaa >= 16) resetFlags |= BGFX_RESET_MSAA_X16;
    else if (msaa >=  8) resetFlags |= BGFX_RESET_MSAA_X8;
    else if (msaa >=  4) resetFlags |= BGFX_RESET_MSAA_X4;
    else if (msaa >=  2) resetFlags |= BGFX_RESET_MSAA_X2;
    init.resolution.reset = resetFlags;
    m_resetFlags = resetFlags;

    if (!bgfx::init(init)) {
        std::fprintf(stderr, "[UILO] bgfx::init failed\n");
        return false;
    }

    bgfx::setDebug(BGFX_DEBUG_NONE);

    m_impl->ensureLayouts();
    if (!m_impl->initShaders()) return false;

    m_msaa        = msaa;
    m_initialised = true;
    return true;
}

void Renderer::shutdown() {
    if (!m_initialised) return;
    if (m_impl) m_impl->shutdownResources();
    bgfx::shutdown();
    if (m_window) { SDL_DestroyWindow(m_window); m_window = nullptr; }
    SDL_Quit();
    m_initialised = false;
}

// ============================================================================
//  Window / cursor
// ============================================================================

Vec2u Renderer::getSize() const {
    if (!m_window) return {0u, 0u};
    int w, h;
    SDL_GetWindowSizeInPixels(m_window, &w, &h);
    return { (unsigned)w, (unsigned)h };
}

void Renderer::setTitle(const std::string& title) {
    if (m_window) SDL_SetWindowTitle(m_window, title.c_str());
}

void Renderer::setVsync(bool enabled) {
    uint32_t f = m_resetFlags;
    if (enabled) f |=  BGFX_RESET_VSYNC;
    else         f &= ~BGFX_RESET_VSYNC;
    if (f == m_resetFlags) return;
    m_resetFlags = f;
    Vec2u sz = getSize();
    bgfx::reset(sz.x, sz.y, m_resetFlags);
    m_lastWidth  = sz.x;
    m_lastHeight = sz.y;
}

bool Renderer::getVsync() const {
    return (m_resetFlags & BGFX_RESET_VSYNC) != 0;
}

void Renderer::setCursor(CursorType type) {
    auto& impl = *m_impl;
    int key = (int)type;
    SDL_Cursor* cur = nullptr;
    auto it = impl.cursors.find(key);
    if (it != impl.cursors.end()) {
        cur = (SDL_Cursor*)it->second;
    } else {
        SDL_SystemCursor sdlType = SDL_SYSTEM_CURSOR_DEFAULT;
        switch (type) {
            case CursorType::Arrow:          sdlType = SDL_SYSTEM_CURSOR_DEFAULT;   break;
            case CursorType::Hand:           sdlType = SDL_SYSTEM_CURSOR_POINTER;   break;
            case CursorType::SizeHorizontal: sdlType = SDL_SYSTEM_CURSOR_EW_RESIZE; break;
            case CursorType::SizeVertical:   sdlType = SDL_SYSTEM_CURSOR_NS_RESIZE; break;
            case CursorType::Text:           sdlType = SDL_SYSTEM_CURSOR_TEXT;      break;
        }
        cur = SDL_CreateSystemCursor(sdlType);
        if (cur) impl.cursors.emplace(key, cur);
    }
    if (cur) SDL_SetCursor(cur);
}

// ============================================================================
//  Frame lifecycle / projection
// ============================================================================

void Renderer::beginFrame() {
    Vec2u sz = getSize();
    if (sz.x != m_lastWidth || sz.y != m_lastHeight) {
        bgfx::reset(sz.x, sz.y, m_resetFlags);
        m_lastWidth  = sz.x;
        m_lastHeight = sz.y;
    }
    bgfx::setViewRect(0, 0, 0, (uint16_t)sz.x, (uint16_t)sz.y);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.f, 0);
    bgfx::setViewMode(0, bgfx::ViewMode::Sequential);
    submitOrtho(0, sz);
    bgfx::touch(0);
}

void Renderer::endFrame() { bgfx::frame(); }

void Renderer::submitOrtho(uint16_t viewId, Vec2u size) {
    const float W  = (float)size.x;
    const float H  = (float)size.y;
    const float nd = 0.f, fd = 1.f;
    const bool  hd = bgfx::getCaps()->homogeneousDepth;
    const float zScale = hd ? 2.f/(fd-nd)      : 1.f/(fd-nd);
    const float zBias  = hd ? -(fd+nd)/(fd-nd) : -nd/(fd-nd);
    const float ortho[16] = {
        2.f/W, 0.f,   0.f,    0.f,
        0.f,  -2.f/H, 0.f,    0.f,
        0.f,   0.f,   zScale, 0.f,
       -1.f,   1.f,   zBias,  1.f
    };
    bgfx::setViewTransform(viewId, nullptr, ortho);
}

uint16_t Renderer::currentViewId() const {
    if (m_viewStackTop > 0) return m_viewStack[m_viewStackTop - 1].viewId;
    return 0;
}

// ============================================================================
//  Framebuffer
// ============================================================================

FrameBuffer Renderer::createFrameBuffer(Vec2u size) {
    FrameBuffer fb;
    fb.size   = size;
    fb.viewId = m_nextViewId++;

    bgfx::FrameBufferHandle h = bgfx::createFrameBuffer(
        (uint16_t)size.x, (uint16_t)size.y,
        bgfx::TextureFormat::BGRA8,
        BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
    fb.handle = h.idx;

    bgfx::setViewFrameBuffer(fb.viewId, h);
    bgfx::setViewRect(fb.viewId, 0, 0, (uint16_t)size.x, (uint16_t)size.y);
    submitOrtho(fb.viewId, size);
    return fb;
}

void Renderer::resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize) {
    if (fb.size == newSize) return;
    destroyFrameBuffer(fb);
    fb = createFrameBuffer(newSize);
}

void Renderer::destroyFrameBuffer(FrameBuffer& fb) {
    if (!fb.valid()) return;
    bgfx::FrameBufferHandle h{ fb.handle };
    bgfx::destroy(h);
    fb.handle = UINT16_MAX;
}

void Renderer::pushFrameBuffer(FrameBuffer& fb) {
    assert(m_viewStackTop < kMaxViewStack);
    m_viewStack[m_viewStackTop++] = { fb.viewId };
    bgfx::setViewRect(fb.viewId, 0, 0, (uint16_t)fb.size.x, (uint16_t)fb.size.y);
    submitOrtho(fb.viewId, fb.size);
}

void Renderer::popFrameBuffer() {
    if (m_viewStackTop > 0) --m_viewStackTop;
}

void Renderer::drawFrameBuffer(const FrameBuffer& fb, Vec2f dest, Vec2f size,
                                Color tint) {
    (void)fb; (void)dest; (void)size; (void)tint;
    // TODO: textured-quad blit of fb's color attachment
}

// ============================================================================
//  Scissor
// ============================================================================

void Renderer::pushScissor(Rectf b) {
    auto& impl = *m_impl;
    if (impl.scissorTop >= Impl::kMaxScissor) return;
    if (impl.scissorTop > 0) {
        auto& p  = impl.scissorStack[impl.scissorTop - 1];
        float px = (float)p.x, py = (float)p.y;
        float px2 = px + (float)p.w, py2 = py + (float)p.h;
        float bx2 = b.position.x + b.size.x, by2 = b.position.y + b.size.y;
        float nx  = std::max(b.position.x, px);
        float ny  = std::max(b.position.y, py);
        float nx2 = std::min(bx2, px2);
        float ny2 = std::min(by2, py2);
        b.position = {nx, ny};
        b.size     = {std::max(0.f, nx2 - nx), std::max(0.f, ny2 - ny)};
    }
    impl.scissorStack[impl.scissorTop++] = {
        (uint16_t)std::max(0.f, b.position.x),
        (uint16_t)std::max(0.f, b.position.y),
        (uint16_t)std::max(0.f, b.size.x),
        (uint16_t)std::max(0.f, b.size.y)
    };
}

void Renderer::popScissor() {
    if (m_impl->scissorTop > 0) --m_impl->scissorTop;
}

void Renderer::clear(Color color) {
    uint32_t rgba = (uint32_t(color.r) << 24) | (uint32_t(color.g) << 16) |
                    (uint32_t(color.b) <<  8) |  uint32_t(color.a);
    bgfx::setViewClear(currentViewId(),
                       BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, rgba, 1.f, 0);
    bgfx::touch(currentViewId());
}

// ============================================================================
//  Shape drawing
// ============================================================================

static constexpr float kDeg2Rad = 3.14159265f / 180.f;

namespace {
inline void applyScissor(const Renderer::Impl& impl) {
    if (impl.scissorTop > 0) {
        auto& sc = impl.scissorStack[impl.scissorTop - 1];
        bgfx::setScissor(sc.x, sc.y, sc.w, sc.h);
    }
}
inline bool scissorEmpty(const Renderer::Impl& impl) {
    if (impl.scissorTop == 0) return false;
    const auto& sc = impl.scissorStack[impl.scissorTop - 1];
    return sc.w == 0 || sc.h == 0;
}
} // anon

void Renderer::draw(const Rect& r) {
    auto& impl = *m_impl;
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;

    uint32_t col = packColor(r.fillColor);
    float x = r.position.x, y = r.position.y;
    float w = r.size.x,     h = r.size.y;

    PosColorVertex verts[4] = {
        {x,     y,     col},
        {x + w, y,     col},
        {x + w, y + h, col},
        {x,     y + h, col},
    };
    uint16_t idx[6] = {0,1,2, 0,2,3};

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.solidLayout, 4, &tib, 6)) return;
    std::memcpy(tvb.data, verts, sizeof(verts));
    std::memcpy(tib.data, idx,   sizeof(idx));
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.solidProgram);

    if (r.outlineThickness > 0.f && r.outlineColor.a > 0) {
        float t = r.outlineThickness;
        Rect top    { r.position,        {w, t}, r.outlineColor };
        Rect bottom { {x, y + h - t},    {w, t}, r.outlineColor };
        Rect left   { r.position,        {t, h}, r.outlineColor };
        Rect right  { {x + w - t, y},    {t, h}, r.outlineColor };
        draw(top); draw(bottom); draw(left); draw(right);
    }
}

void Renderer::draw(const RoundedRect& rr) {
    auto& impl = *m_impl;
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;

    int   segs = std::max(2, rr.cornerSegments);
    float r    = std::min(rr.radius, std::min(rr.size.x, rr.size.y) * 0.5f);
    if (r <= 0.f) {
        draw(Rect{rr.position, rr.size, rr.fillColor,
                  rr.outlineColor, rr.outlineThickness});
        return;
    }

    uint32_t col          = packColor(rr.fillColor);
    uint32_t nCornerVerts = (uint32_t)segs * 4;
    uint32_t nVerts       = nCornerVerts + 1;
    uint32_t nIdx         = nCornerVerts * 3;

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.solidLayout, nVerts, &tib, nIdx))
        return;

    auto* verts   = reinterpret_cast<PosColorVertex*>(tvb.data);
    auto* indices = reinterpret_cast<uint16_t*>(tib.data);

    float cx = rr.position.x + rr.size.x * 0.5f;
    float cy = rr.position.y + rr.size.y * 0.5f;
    verts[0] = {cx, cy, col};

    struct CornerDef { float ox, oy, startDeg, endDeg; };
    float x = rr.position.x, y = rr.position.y;
    float w = rr.size.x,     h = rr.size.y;
    const CornerDef corners[4] = {
        { w - r, r,     -90.f,   0.f },
        { w - r, h - r,   0.f,  90.f },
        { r,     h - r,  90.f, 180.f },
        { r,     r,     180.f, 270.f },
    };

    uint32_t vi = 1, ii = 0;
    for (const auto& c : corners) {
        for (int s = 0; s < segs; ++s, ++vi) {
            float t     = (float)s / (float)(segs - 1);
            float angle = (c.startDeg + t * (c.endDeg - c.startDeg)) * kDeg2Rad;
            verts[vi] = {
                x + c.ox + r * std::cos(angle),
                y + c.oy + r * std::sin(angle),
                col
            };
            if (vi > 1) {
                indices[ii++] = 0;
                indices[ii++] = (uint16_t)(vi - 1);
                indices[ii++] = (uint16_t)(vi);
            }
        }
    }
    indices[ii++] = 0;
    indices[ii++] = (uint16_t)(nVerts - 1);
    indices[ii++] = 1;

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.solidProgram);

    if (rr.outlineThickness > 0.f && rr.outlineColor.a > 0) {
        // Outline approximated as four solid Rects around an inner rounded
        // shape would leave gaps at corners; instead draw a slightly larger
        // rounded rect underneath in the outline color.
        RoundedRect bg = rr;
        bg.fillColor        = rr.outlineColor;
        bg.outlineThickness = 0.f;
        bg.position.x -= rr.outlineThickness;
        bg.position.y -= rr.outlineThickness;
        bg.size.x     += rr.outlineThickness * 2.f;
        bg.size.y     += rr.outlineThickness * 2.f;
        bg.radius     += rr.outlineThickness;
        // Draw outline first, then redraw fill on top.
        // Re-entrant call so the original fill path runs again without the
        // outline branch (outlineThickness==0).
        RoundedRect fill = rr;
        fill.outlineThickness = 0.f;
        draw(bg);
        draw(fill);
    }
}

void Renderer::draw(const Circle& c) {
    auto& impl = *m_impl;
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;

    int segs       = std::max(3, c.segments);
    uint32_t col   = packColor(c.fillColor);
    uint32_t nV    = (uint32_t)segs + 1;
    uint32_t nIdx  = (uint32_t)segs * 3;

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.solidLayout, nV, &tib, nIdx))
        return;

    auto* verts   = reinterpret_cast<PosColorVertex*>(tvb.data);
    auto* indices = reinterpret_cast<uint16_t*>(tib.data);

    verts[0] = {c.center.x, c.center.y, col};
    for (int i = 0; i < segs; ++i) {
        float angle = (float)i / (float)segs * 360.f * kDeg2Rad;
        verts[i + 1] = {
            c.center.x + c.radius * std::cos(angle),
            c.center.y + c.radius * std::sin(angle),
            col
        };
        indices[i*3 + 0] = 0;
        indices[i*3 + 1] = (uint16_t)(i + 1);
        indices[i*3 + 2] = (uint16_t)((i + 1) % segs + 1);
    }

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.solidProgram);
}

void Renderer::draw(const Triangle& t) {
    auto& impl = *m_impl;
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;

    uint32_t col = packColor(t.fillColor);
    PosColorVertex verts[3] = {
        {t.a.x, t.a.y, col},
        {t.b.x, t.b.y, col},
        {t.c.x, t.c.y, col},
    };
    uint16_t idx[3] = {0,1,2};

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.solidLayout, 3, &tib, 3))
        return;
    std::memcpy(tvb.data, verts, sizeof(verts));
    std::memcpy(tib.data, idx,   sizeof(idx));
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.solidProgram);
}

void Renderer::draw(const Line& l) {
    auto& impl = *m_impl;
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;

    float dx = l.end.x - l.start.x;
    float dy = l.end.y - l.start.y;
    float len = std::sqrt(dx*dx + dy*dy);
    if (len < 0.001f) return;

    float nx = -dy / len * l.thickness * 0.5f;
    float ny =  dx / len * l.thickness * 0.5f;
    uint32_t col = packColor(l.color);

    PosColorVertex verts[4] = {
        {l.start.x + nx, l.start.y + ny, col},
        {l.start.x - nx, l.start.y - ny, col},
        {l.end.x   - nx, l.end.y   - ny, col},
        {l.end.x   + nx, l.end.y   + ny, col},
    };
    uint16_t idx[6] = {0,1,2, 0,2,3};

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.solidLayout, 4, &tib, 6))
        return;
    std::memcpy(tvb.data, verts, sizeof(verts));
    std::memcpy(tib.data, idx,   sizeof(idx));
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.solidProgram);
}

} // namespace uilo
