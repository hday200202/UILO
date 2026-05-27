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
#include <chrono>
#include <cmath>
#include <thread>

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
#include "spirv/fs_blur.sc.bin.h"
#include "spirv/fs_glass.sc.bin.h"

#include "glsl/vs_solid.sc.bin.h"
#include "glsl/vs_tex.sc.bin.h"
#include "glsl/fs_solid.sc.bin.h"
#include "glsl/fs_tex.sc.bin.h"
#include "glsl/fs_text.sc.bin.h"
#include "glsl/fs_blur.sc.bin.h"
#include "glsl/fs_glass.sc.bin.h"

#include "essl/vs_solid.sc.bin.h"
#include "essl/vs_tex.sc.bin.h"
#include "essl/fs_solid.sc.bin.h"
#include "essl/fs_tex.sc.bin.h"
#include "essl/fs_text.sc.bin.h"
#include "essl/fs_blur.sc.bin.h"
#include "essl/fs_glass.sc.bin.h"

#if BX_PLATFORM_OSX || BX_PLATFORM_IOS
#  include "metal/vs_solid.sc.bin.h"
#  include "metal/vs_tex.sc.bin.h"
#  include "metal/fs_solid.sc.bin.h"
#  include "metal/fs_tex.sc.bin.h"
#  include "metal/fs_text.sc.bin.h"
#  include "metal/fs_blur.sc.bin.h"
#  include "metal/fs_glass.sc.bin.h"
#endif

#if BX_PLATFORM_WINDOWS
#  include "dx11/vs_solid.sc.bin.h"
#  include "dx11/vs_tex.sc.bin.h"
#  include "dx11/fs_solid.sc.bin.h"
#  include "dx11/fs_tex.sc.bin.h"
#  include "dx11/fs_text.sc.bin.h"
#  include "dx11/fs_blur.sc.bin.h"
#  include "dx11/fs_glass.sc.bin.h"
#endif

namespace uilo {

namespace {
    static const bgfx::EmbeddedShader s_embeddedShaders[] = {
        BGFX_EMBEDDED_SHADER(vs_solid),
        BGFX_EMBEDDED_SHADER(fs_solid),
        BGFX_EMBEDDED_SHADER(vs_tex),
        BGFX_EMBEDDED_SHADER(fs_tex),
        BGFX_EMBEDDED_SHADER(fs_text),
        BGFX_EMBEDDED_SHADER(fs_blur),
        BGFX_EMBEDDED_SHADER(fs_glass),
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
    bgfx::ShaderHandle vst3 = bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_tex");
    bgfx::ShaderHandle vst4 = bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_tex");
    bgfx::ShaderHandle fst  = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_tex");
    bgfx::ShaderHandle ftx  = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_text");
    bgfx::ShaderHandle fbl  = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_blur");
    bgfx::ShaderHandle fgl  = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_glass");
    if (!bgfx::isValid(vs)   || !bgfx::isValid(fs)   ||
        !bgfx::isValid(vst1) || !bgfx::isValid(vst2) ||
        !bgfx::isValid(vst3) || !bgfx::isValid(vst4) ||
        !bgfx::isValid(fst)  || !bgfx::isValid(ftx)  ||
        !bgfx::isValid(fbl)  || !bgfx::isValid(fgl)) {
        std::fprintf(stderr, "[UILO] Failed to create shaders (renderer=%s)\n",
                     bgfx::getRendererName(type));
        return false;
    }
    solidProgram = bgfx::createProgram(vs,   fs,  true);
    texProgram   = bgfx::createProgram(vst1, fst, true);
    textProgram  = bgfx::createProgram(vst2, ftx, true);
    blurProgram  = bgfx::createProgram(vst3, fbl, true);
    glassProgram = bgfx::createProgram(vst4, fgl, true);
    s_texColor   = bgfx::createUniform("s_texColor",   bgfx::UniformType::Sampler);
    u_imgFlags   = bgfx::createUniform("u_imgFlags",   bgfx::UniformType::Vec4);
    u_blurParams = bgfx::createUniform("u_blurParams", bgfx::UniformType::Vec4);
    u_glassParams= bgfx::createUniform("u_glassParams",bgfx::UniformType::Vec4);
    u_glassTint  = bgfx::createUniform("u_glassTint",  bgfx::UniformType::Vec4);
    u_glassRect  = bgfx::createUniform("u_glassRect",  bgfx::UniformType::Vec4);
    u_glassAnim  = bgfx::createUniform("u_glassAnim",  bgfx::UniformType::Vec4);
    u_glassBase  = bgfx::createUniform("u_glassBase",  bgfx::UniformType::Vec4);
    u_glassMouse = bgfx::createUniform("u_glassMouse", bgfx::UniformType::Vec4);
    u_clipRect   = bgfx::createUniform("u_clipRect",   bgfx::UniformType::Vec4);
    u_clipParams = bgfx::createUniform("u_clipParams", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(solidProgram) ||
        !bgfx::isValid(texProgram)   ||
        !bgfx::isValid(textProgram)  ||
        !bgfx::isValid(blurProgram)  ||
        !bgfx::isValid(glassProgram)) {
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
    if (bgfx::isValid(u_imgFlags))   bgfx::destroy(u_imgFlags);
    if (bgfx::isValid(u_blurParams)) bgfx::destroy(u_blurParams);
    if (bgfx::isValid(u_glassParams))bgfx::destroy(u_glassParams);
    if (bgfx::isValid(u_glassTint))  bgfx::destroy(u_glassTint);
    if (bgfx::isValid(u_glassRect))  bgfx::destroy(u_glassRect);
    if (bgfx::isValid(u_glassAnim))  bgfx::destroy(u_glassAnim);
    if (bgfx::isValid(u_glassBase))  bgfx::destroy(u_glassBase);
    if (bgfx::isValid(u_glassMouse)) bgfx::destroy(u_glassMouse);
    if (bgfx::isValid(u_clipRect))   bgfx::destroy(u_clipRect);
    if (bgfx::isValid(u_clipParams)) bgfx::destroy(u_clipParams);
    if (bgfx::isValid(solidProgram)) bgfx::destroy(solidProgram);
    if (bgfx::isValid(texProgram))   bgfx::destroy(texProgram);
    if (bgfx::isValid(textProgram))  bgfx::destroy(textProgram);
    if (bgfx::isValid(blurProgram))  bgfx::destroy(blurProgram);
    if (bgfx::isValid(glassProgram)) bgfx::destroy(glassProgram);
    destroySceneFramebuffers();
    s_texColor   = BGFX_INVALID_HANDLE;
    solidProgram = BGFX_INVALID_HANDLE;
    texProgram   = BGFX_INVALID_HANDLE;
    textProgram  = BGFX_INVALID_HANDLE;
    u_imgFlags   = BGFX_INVALID_HANDLE;
    blurProgram  = BGFX_INVALID_HANDLE;
    glassProgram = BGFX_INVALID_HANDLE;
    u_blurParams = BGFX_INVALID_HANDLE;
    u_glassParams= BGFX_INVALID_HANDLE;
    u_glassTint  = BGFX_INVALID_HANDLE;
    u_glassRect  = BGFX_INVALID_HANDLE;
    u_glassAnim  = BGFX_INVALID_HANDLE;
    u_glassBase  = BGFX_INVALID_HANDLE;
    u_glassMouse = BGFX_INVALID_HANDLE;
}

// ============================================================================
//  Offscreen scene + blur ladder (Material::Glass)
// ============================================================================

void Renderer::Impl::destroySceneFramebuffers() {
    if (bgfx::isValid(sceneFB))  bgfx::destroy(sceneFB);
    if (bgfx::isValid(blurFB_A)) bgfx::destroy(blurFB_A);
    if (bgfx::isValid(blurFB_B)) bgfx::destroy(blurFB_B);
    sceneFB  = BGFX_INVALID_HANDLE;
    blurFB_A = BGFX_INVALID_HANDLE;
    blurFB_B = BGFX_INVALID_HANDLE;
    sceneColorTex = BGFX_INVALID_HANDLE;
    blurColorA    = BGFX_INVALID_HANDLE;
    blurColorB    = BGFX_INVALID_HANDLE;
    fbWidth = fbHeight = 0;
}

void Renderer::Impl::ensureSceneFramebuffers(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (bgfx::isValid(sceneFB) && fbWidth == width && fbHeight == height) return;

    destroySceneFramebuffers();

    const uint64_t fbFlags = BGFX_TEXTURE_RT
                           | BGFX_SAMPLER_U_CLAMP
                           | BGFX_SAMPLER_V_CLAMP
                           | BGFX_SAMPLER_MIN_ANISOTROPIC
                           | BGFX_SAMPLER_MAG_ANISOTROPIC;

    // Full-res scene target.
    sceneFB = bgfx::createFrameBuffer(
        (uint16_t)width, (uint16_t)height,
        bgfx::TextureFormat::BGRA8, fbFlags);
    sceneColorTex = bgfx::getTexture(sceneFB, 0);

    // Half-res ping/pong blur targets. Min 1px to avoid 0-sized FBs.
    const uint16_t halfW = (uint16_t)std::max(1u, width  / 2u);
    const uint16_t halfH = (uint16_t)std::max(1u, height / 2u);

    blurFB_A = bgfx::createFrameBuffer(halfW, halfH,
                                       bgfx::TextureFormat::BGRA8, fbFlags);
    blurFB_B = bgfx::createFrameBuffer(halfW, halfH,
                                       bgfx::TextureFormat::BGRA8, fbFlags);
    blurColorA = bgfx::getTexture(blurFB_A, 0);
    blurColorB = bgfx::getTexture(blurFB_B, 0);

    fbWidth  = width;
    fbHeight = height;

    // Bind FBs to their reserved view IDs. View 0 (scene) clears, blur views
    // overwrite, composite writes backbuffer (FB = invalid).
    bgfx::setViewFrameBuffer(kSceneViewId,     sceneFB);
    bgfx::setViewFrameBuffer(kBlurHViewId,     blurFB_A);
    bgfx::setViewFrameBuffer(kBlurVViewId,     blurFB_B);
    bgfx::setViewFrameBuffer(kCompositeViewId, BGFX_INVALID_HANDLE);

    // Static view rects (they don't change between frames at this size).
    bgfx::setViewRect(kSceneViewId, 0, 0, (uint16_t)width, (uint16_t)height);
    bgfx::setViewRect(kBlurHViewId, 0, 0, halfW, halfH);
    bgfx::setViewRect(kBlurVViewId, 0, 0, halfW, halfH);
    bgfx::setViewRect(kCompositeViewId, 0, 0, (uint16_t)width, (uint16_t)height);

    // Composite + blur views always go straight through (no depth, no clear).
    bgfx::setViewClear(kBlurHViewId, BGFX_CLEAR_NONE);
    bgfx::setViewClear(kBlurVViewId, BGFX_CLEAR_NONE);
    bgfx::setViewClear(kCompositeViewId, BGFX_CLEAR_NONE);
    bgfx::setViewMode(kBlurHViewId,     bgfx::ViewMode::Sequential);
    bgfx::setViewMode(kBlurVViewId,     bgfx::ViewMode::Sequential);
    bgfx::setViewMode(kCompositeViewId, bgfx::ViewMode::Sequential);
}

namespace {
    // Submit a full-screen textured quad covering [0,0]-[w,h] in pixels with
    // UVs [0,0]-[1,1]. Vertex color is white. Caller is responsible for
    // setting texture, uniforms, state, and view.
    void submitFullscreenQuad(const bgfx::VertexLayout& layout,
                              uint16_t viewId,
                              float dstW, float dstH,
                              bgfx::ProgramHandle program,
                              bool flipV) {
        bgfx::TransientVertexBuffer tvb;
        if (bgfx::getAvailTransientVertexBuffer(6, layout) < 6) return;
        bgfx::allocTransientVertexBuffer(&tvb, 6, layout);

        struct V { float x, y; uint32_t abgr; float u, v; };
        V* v = (V*)tvb.data;
        const uint32_t white = 0xffffffffu;
        const float v0 = flipV ? 1.f : 0.f;
        const float v1 = flipV ? 0.f : 1.f;

        // Triangle 1
        v[0] = { 0.f,  0.f,  white, 0.f, v0 };
        v[1] = { dstW, 0.f,  white, 1.f, v0 };
        v[2] = { dstW, dstH, white, 1.f, v1 };
        // Triangle 2
        v[3] = { 0.f,  0.f,  white, 0.f, v0 };
        v[4] = { dstW, dstH, white, 1.f, v1 };
        v[5] = { 0.f,  dstH, white, 0.f, v1 };

        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
        bgfx::submit(viewId, program);
    }
}

void Renderer::Impl::runBlurPasses(uint32_t width, uint32_t height) {
    if (!bgfx::isValid(sceneFB) || !bgfx::isValid(blurProgram)) return;
    const uint16_t halfW = (uint16_t)std::max(1u, width  / 2u);
    const uint16_t halfH = (uint16_t)std::max(1u, height / 2u);

    const bool flipV = bgfx::getCaps()->originBottomLeft;

    // Set ortho for blur views matching their FB sizes.
    auto setOrtho = [](uint16_t viewId, uint32_t w, uint32_t h) {
        const float W = (float)w, H = (float)h;
        const bool  hd = bgfx::getCaps()->homogeneousDepth;
        const float zScale = hd ? 2.f : 1.f;
        const float zBias  = hd ? -1.f : 0.f;
        const float m[16] = {
            2.f/W, 0.f,   0.f,   0.f,
            0.f,  -2.f/H, 0.f,   0.f,
            0.f,   0.f,   zScale,0.f,
           -1.f,   1.f,   zBias, 1.f
        };
        bgfx::setViewTransform(viewId, nullptr, m);
    };
    setOrtho(kBlurHViewId, halfW, halfH);
    setOrtho(kBlurVViewId, halfW, halfH);

    // ---- Horizontal blur: sceneFB color -> blurFB_A ----
    {
        const float step[4] = { 1.f / (float)halfW, 0.f, 0.f, 0.f };
        bgfx::setUniform(u_blurParams, step);
        bgfx::setTexture(0, s_texColor, sceneColorTex);
        submitFullscreenQuad(texLayout, kBlurHViewId,
                             (float)halfW, (float)halfH,
                             blurProgram, flipV);
    }
    // ---- Vertical blur: blurFB_A -> blurFB_B ----
    {
        const float step[4] = { 0.f, 1.f / (float)halfH, 0.f, 0.f };
        bgfx::setUniform(u_blurParams, step);
        bgfx::setTexture(0, s_texColor, blurColorA);
        submitFullscreenQuad(texLayout, kBlurVViewId,
                             (float)halfW, (float)halfH,
                             blurProgram, false /* sampling our own RT in same orient */);
    }
}

void Renderer::Impl::compositeSceneToBackbuffer(uint32_t width, uint32_t height,
                                                const bgfx::VertexLayout& layout,
                                                bgfx::ProgramHandle program) {
    if (!bgfx::isValid(sceneFB)) return;
    const bool flipV = bgfx::getCaps()->originBottomLeft;

    const float W = (float)width, H = (float)height;
    const bool  hd = bgfx::getCaps()->homogeneousDepth;
    const float zScale = hd ? 2.f : 1.f;
    const float zBias  = hd ? -1.f : 0.f;
    const float ortho[16] = {
        2.f/W, 0.f,   0.f,    0.f,
        0.f,  -2.f/H, 0.f,    0.f,
        0.f,   0.f,   zScale, 0.f,
       -1.f,   1.f,   zBias,  1.f
    };
    bgfx::setViewTransform(kCompositeViewId, nullptr, ortho);

    // For the composite step we want the fs_tex shader path so we need to
    // also reset the u_imgFlags uniform (no ellipse clipping).
    const float flags[4] = { 0.f, 0.f, 0.f, 0.f };
    bgfx::setUniform(u_imgFlags, flags);

    // The full-screen blit must not inherit a stale rounded-clip from the
    // last in-scene draw call. Reset clip uniforms to "disabled".
    const float clipZero[4] = { 0.f, 0.f, 0.f, 0.f };
    if (bgfx::isValid(u_clipRect))   bgfx::setUniform(u_clipRect,   clipZero);
    if (bgfx::isValid(u_clipParams)) bgfx::setUniform(u_clipParams, clipZero);

    bgfx::setTexture(0, s_texColor, sceneColorTex);
    submitFullscreenQuad(layout, kCompositeViewId, W, H, program, flipV);
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
    // Limit swapchain queued frames (Metal/DXGI) to 1 to minimise input lag.
    init.resolution.maxFrameLatency = 1;
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
    // maxFrameLatency only applies on full reset with width/height; keep 1.
    bgfx::reset(sz.x, sz.y, m_resetFlags);
    m_lastWidth  = sz.x;
    m_lastHeight = sz.y;
}

bool Renderer::getVsync() const {
    return (m_resetFlags & BGFX_RESET_VSYNC) != 0;
}

void Renderer::setFramerateLimit(float fps) {
    if (fps <= 0.f || !std::isfinite(fps)) {
        m_frameInterval = 0.0;
        m_nextFrameTick = 0;
        return;
    }
    m_frameInterval = 1.0 / (double)fps;
    m_nextFrameTick = 0; // re-anchor on next endFrame()
}

float Renderer::getFramerateLimit() const {
    return m_frameInterval > 0.0 ? (float)(1.0 / m_frameInterval) : 0.f;
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
    // Wall-clock timer for animated materials. Use real time (not dt) so
    // animation rate is independent of vsync / framerate-cap.
    {
        using clock = std::chrono::steady_clock;
        static const auto s_t0 = clock::now();
        const auto now = clock::now();
        m_impl->elapsed = std::chrono::duration<float>(now - s_t0).count();
    }
    // (Re)create offscreen scene + blur framebuffers if the window resized.
    m_impl->ensureSceneFramebuffers(sz.x, sz.y);

    // View 0 → sceneFB (set every frame in case bgfx reset clobbered it).
    bgfx::setViewFrameBuffer(0, m_impl->sceneFB);
    bgfx::setViewRect(0, 0, 0, (uint16_t)sz.x, (uint16_t)sz.y);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.f, 0);
    bgfx::setViewMode(0, bgfx::ViewMode::Sequential);
    submitOrtho(0, sz);
    bgfx::touch(0);
}

void Renderer::endFrame() {
    // After the scene has been submitted to sceneFB, run the blur ladder
    // (consumed by glass elements next frame) and composite sceneFB to the
    // backbuffer.
    Vec2u sz = getSize();
    m_impl->runBlurPasses(sz.x, sz.y);
    m_impl->compositeSceneToBackbuffer(sz.x, sz.y,
                                       m_impl->texLayout,
                                       m_impl->texProgram);

    bgfx::frame();
    if (m_frameInterval > 0.0) {
        using clock = std::chrono::steady_clock;
        const auto now    = clock::now();
        const auto nowNs  = (uint64_t)std::chrono::duration_cast<
                                std::chrono::nanoseconds>(now.time_since_epoch()).count();
        const uint64_t intervalNs = (uint64_t)(m_frameInterval * 1e9);
        if (m_nextFrameTick == 0 || nowNs > m_nextFrameTick + intervalNs) {
            // First call or we drifted way behind: re-anchor.
            m_nextFrameTick = nowNs + intervalNs;
        } else {
            if (nowNs < m_nextFrameTick) {
                const uint64_t waitNs = m_nextFrameTick - nowNs;
                // Sleep most of the remaining time, leaving ~0.5 ms for a
                // short spin so we don't overshoot due to scheduler latency.
                constexpr uint64_t spinMarginNs = 500'000; // 0.5 ms
                if (waitNs > spinMarginNs) {
                    std::this_thread::sleep_for(
                        std::chrono::nanoseconds(waitNs - spinMarginNs));
                }
                while ((uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                           clock::now().time_since_epoch()).count() < m_nextFrameTick) {
                    std::this_thread::yield();
                }
            }
            m_nextFrameTick += intervalNs;
        }
    }
}

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

void Renderer::pushRoundClip(Rectf b, float radius) {
    auto& impl = *m_impl;
    pushScissor(b);
    if (impl.roundClipTop >= Impl::kMaxRoundClip) return;
    float r = std::max(0.f, radius);
    if (r <= 0.f && impl.roundClipTop == 0) {
        // Plain rectangle: still record an "off" entry so pop balances.
        impl.roundClipStack[impl.roundClipTop++] = {0.f, 0.f, 0.f, 0.f, -1.f};
        return;
    }
    float halfW = b.size.x * 0.5f;
    float halfH = b.size.y * 0.5f;
    float cx    = b.position.x + halfW;
    float cy    = b.position.y + halfH;
    // Intersect with the parent rounded clip (if any) by shrinking the
    // smaller of the two rects. A perfect intersection of two rounded rects
    // is hairy; in practice nested rounded containers are concentric or
    // the child stays well inside the parent so the inner clip dominates.
    if (impl.roundClipTop > 0) {
        const auto& p = impl.roundClipStack[impl.roundClipTop - 1];
        if (p.radius >= 0.f) {
            float pCx = p.cx, pCy = p.cy, pHw = p.halfW, pHh = p.halfH;
            float nx1 = std::max(cx - halfW, pCx - pHw);
            float ny1 = std::max(cy - halfH, pCy - pHh);
            float nx2 = std::min(cx + halfW, pCx + pHw);
            float ny2 = std::min(cy + halfH, pCy + pHh);
            halfW = std::max(0.f, (nx2 - nx1) * 0.5f);
            halfH = std::max(0.f, (ny2 - ny1) * 0.5f);
            cx    = (nx1 + nx2) * 0.5f;
            cy    = (ny1 + ny2) * 0.5f;
            r     = std::min(r, std::min(halfW, halfH));
        }
    }
    r = std::min(r, std::min(halfW, halfH));
    impl.roundClipStack[impl.roundClipTop++] = {cx, cy, halfW, halfH, r};
}

void Renderer::popRoundClip() {
    auto& impl = *m_impl;
    if (impl.roundClipTop > 0) --impl.roundClipTop;
    popScissor();
}

void Renderer::setMouseState(Vec2f mousePosFbPx) {
    // Detect motion vs the previous frame so animated materials can fade
    // the ripple amplitude when the cursor sits still. Threshold avoids
    // sub-pixel noise from triggering constant "moving" state.
    const float dx = mousePosFbPx.x - m_mousePosPrev.x;
    const float dy = mousePosFbPx.y - m_mousePosPrev.y;
    if (dx * dx + dy * dy > 0.25f) {
        m_mouseLastMoveT = m_impl->elapsed;
    }
    m_mousePosPrev = m_mousePos;
    m_mousePos     = mousePosFbPx;
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
inline void applyRoundClipInner(const Renderer::Impl& impl) {
    float rect[4]   = {0.f, 0.f, 0.f, 0.f};
    float params[4] = {0.f, 0.f, 0.f, 0.f};
    if (impl.roundClipTop > 0) {
        const auto& c = impl.roundClipStack[impl.roundClipTop - 1];
        if (c.radius > 0.f) {
            rect[0] = c.cx; rect[1] = c.cy;
            rect[2] = c.halfW; rect[3] = c.halfH;
            params[0] = c.radius; params[1] = 1.f;
        }
    }
    if (bgfx::isValid(impl.u_clipRect))   bgfx::setUniform(impl.u_clipRect,   rect);
    if (bgfx::isValid(impl.u_clipParams)) bgfx::setUniform(impl.u_clipParams, params);
}
inline void applyScissor(const Renderer::Impl& impl) {
    if (impl.scissorTop > 0) {
        auto& sc = impl.scissorStack[impl.scissorTop - 1];
        bgfx::setScissor(sc.x, sc.y, sc.w, sc.h);
    }
    applyRoundClipInner(impl);
}
inline void applyRoundClip(const Renderer::Impl& impl) {
    applyRoundClipInner(impl);
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

    float r = std::min(rr.radius, std::min(rr.size.x, rr.size.y) * 0.5f);
    if (r <= 0.f) {
        draw(Rect{rr.position, rr.size, rr.fillColor,
                  rr.outlineColor, rr.outlineThickness});
        return;
    }

    // Render the outline as a slightly larger rounded rect underneath, then
    // the fill on top. Each is a single quad masked by the fragment-shader
    // SDF clip — anti-aliased corners with no per-corner tessellation.
    auto submitQuad = [&](float x, float y, float w, float h, Color color) {
        uint32_t col = packColor(color);
        PosColorVertex verts[4] = {
            {x,     y,     col},
            {x + w, y,     col},
            {x + w, y + h, col},
            {x,     y + h, col},
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
    };

    if (rr.outlineThickness > 0.f && rr.outlineColor.a > 0) {
        const float t  = rr.outlineThickness;
        const float ox = rr.position.x - t;
        const float oy = rr.position.y - t;
        const float ow = rr.size.x + t * 2.f;
        const float oh = rr.size.y + t * 2.f;
        const float orad = r + t;
        pushRoundClip({{ox, oy}, {ow, oh}}, orad);
        submitQuad(ox, oy, ow, oh, rr.outlineColor);
        popRoundClip();
    }

    pushRoundClip({rr.position, rr.size}, r);
    submitQuad(rr.position.x, rr.position.y, rr.size.x, rr.size.y, rr.fillColor);
    popRoundClip();
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

void Renderer::drawLines(const Line* lines, size_t count) {
    auto& impl = *m_impl;
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;
    if (!lines || count == 0) return;

    // Worst case all `count` lines are non-degenerate. Cap so we never
    // exceed the 16-bit index range (max 65535 indices => 10922 quads).
    constexpr size_t kMaxQuadsPerBatch = 65535 / 6;
    size_t offset = 0;
    while (offset < count) {
        size_t chunk = std::min(count - offset, kMaxQuadsPerBatch);

        // First pass: count non-degenerate lines so we allocate tightly.
        size_t valid = 0;
        for (size_t i = 0; i < chunk; ++i) {
            const Line& l = lines[offset + i];
            float dx = l.end.x - l.start.x;
            float dy = l.end.y - l.start.y;
            if (dx*dx + dy*dy >= 1e-6f) ++valid;
        }
        if (valid == 0) { offset += chunk; continue; }

        const uint32_t nV = (uint32_t)(valid * 4);
        const uint32_t nI = (uint32_t)(valid * 6);
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer  tib;
        if (!bgfx::allocTransientBuffers(&tvb, impl.solidLayout, nV, &tib, nI))
            return;

        auto* verts = reinterpret_cast<PosColorVertex*>(tvb.data);
        auto* idx   = reinterpret_cast<uint16_t*>(tib.data);

        uint32_t vi = 0, ii = 0;
        for (size_t i = 0; i < chunk; ++i) {
            const Line& l = lines[offset + i];
            float dx = l.end.x - l.start.x;
            float dy = l.end.y - l.start.y;
            float len2 = dx*dx + dy*dy;
            if (len2 < 1e-6f) continue;
            float len = std::sqrt(len2);
            float nx = -dy / len * l.thickness * 0.5f;
            float ny =  dx / len * l.thickness * 0.5f;
            uint32_t col = packColor(l.color);
            verts[vi+0] = {l.start.x + nx, l.start.y + ny, col};
            verts[vi+1] = {l.start.x - nx, l.start.y - ny, col};
            verts[vi+2] = {l.end.x   - nx, l.end.y   - ny, col};
            verts[vi+3] = {l.end.x   + nx, l.end.y   + ny, col};
            idx[ii+0] = (uint16_t)(vi+0);
            idx[ii+1] = (uint16_t)(vi+1);
            idx[ii+2] = (uint16_t)(vi+2);
            idx[ii+3] = (uint16_t)(vi+0);
            idx[ii+4] = (uint16_t)(vi+2);
            idx[ii+5] = (uint16_t)(vi+3);
            vi += 4; ii += 6;
        }

        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setIndexBuffer(&tib);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                       BGFX_STATE_BLEND_ALPHA);
        applyScissor(impl);
        bgfx::submit(currentViewId(), impl.solidProgram);

        offset += chunk;
    }
}

} // namespace uilo
