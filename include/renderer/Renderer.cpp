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
    u_clipRect2  = bgfx::createUniform("u_clipRect2",  bgfx::UniformType::Vec4);
    u_clipParams2= bgfx::createUniform("u_clipParams2",bgfx::UniformType::Vec4);
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
    if (bgfx::isValid(u_clipRect2))  bgfx::destroy(u_clipRect2);
    if (bgfx::isValid(u_clipParams2))bgfx::destroy(u_clipParams2);
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
                           | BGFX_SAMPLER_V_CLAMP;

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
    bgfx::setViewFrameBuffer(kSceneViewId,        sceneFB);
    bgfx::setViewFrameBuffer(kBlurHViewId,        blurFB_A);
    bgfx::setViewFrameBuffer(kBlurVViewId,        blurFB_B);
    bgfx::setViewFrameBuffer(kGlassBgViewId,      sceneFB);
    bgfx::setViewFrameBuffer(kGlassChildViewId,   sceneFB);
    bgfx::setViewFrameBuffer(kCompositeViewId,    BGFX_INVALID_HANDLE);

    // Static view rects (they don't change between frames at this size).
    bgfx::setViewRect(kSceneViewId,      0, 0, (uint16_t)width, (uint16_t)height);
    bgfx::setViewRect(kBlurHViewId,      0, 0, halfW, halfH);
    bgfx::setViewRect(kBlurVViewId,      0, 0, halfW, halfH);
    bgfx::setViewRect(kGlassBgViewId,    0, 0, (uint16_t)width, (uint16_t)height);
    bgfx::setViewRect(kGlassChildViewId, 0, 0, (uint16_t)width, (uint16_t)height);
    bgfx::setViewRect(kCompositeViewId,  0, 0, (uint16_t)width, (uint16_t)height);

    // Composite + blur + glass views always go straight through (no depth, no clear).
    bgfx::setViewClear(kBlurHViewId,        BGFX_CLEAR_NONE);
    bgfx::setViewClear(kBlurVViewId,        BGFX_CLEAR_NONE);
    bgfx::setViewClear(kGlassBgViewId,      BGFX_CLEAR_NONE);
    bgfx::setViewClear(kGlassChildViewId,   BGFX_CLEAR_NONE);
    bgfx::setViewClear(kCompositeViewId,    BGFX_CLEAR_NONE);
    bgfx::setViewMode(kBlurHViewId,         bgfx::ViewMode::Sequential);
    bgfx::setViewMode(kBlurVViewId,         bgfx::ViewMode::Sequential);
    bgfx::setViewMode(kGlassBgViewId,       bgfx::ViewMode::Sequential);
    bgfx::setViewMode(kGlassChildViewId,    bgfx::ViewMode::Sequential);
    bgfx::setViewMode(kCompositeViewId,     bgfx::ViewMode::Sequential);

    // Ortho transforms for glass views (full-window pixel space).
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
    bgfx::setViewTransform(kGlassBgViewId,    nullptr, ortho);
    bgfx::setViewTransform(kGlassChildViewId, nullptr, ortho);
}

namespace {
    // Submit a full-screen textured quad covering [0,0]-[w,h] in pixels with
    // UVs [0,0]-[1,1]. Vertex color is white. Caller is responsible for
    // setting texture, uniforms, state, and view.
    void submitFullscreenQuad(const bgfx::VertexLayout& layout,
                              uint16_t viewId,
                              float dstW, float dstH,
                              bgfx::ProgramHandle program,
                              bool flipV,
                              bool alphaBlend = false) {
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
        uint64_t st = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
        if (alphaBlend) st |= BGFX_STATE_BLEND_ALPHA; // overlay over the host image
        bgfx::setState(st);
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
    if (bgfx::isValid(u_clipRect))    bgfx::setUniform(u_clipRect,    clipZero);
    if (bgfx::isValid(u_clipParams))  bgfx::setUniform(u_clipParams,  clipZero);
    if (bgfx::isValid(u_clipRect2))   bgfx::setUniform(u_clipRect2,   clipZero);
    if (bgfx::isValid(u_clipParams2)) bgfx::setUniform(u_clipParams2, clipZero);

    bgfx::setTexture(0, s_texColor, sceneColorTex);
    submitFullscreenQuad(layout, kCompositeViewId, W, H, program, flipV, embedded);
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

    // Raise transient buffer budgets so dense UI passes (grids, markers,
    // waveform-like primitives) don't hit allocation cliffs and silently drop
    // draws mid-frame.
    constexpr uint32_t kTransientVbBytes = 32u * 1024u * 1024u;
    constexpr uint32_t kTransientIbBytes =  8u * 1024u * 1024u;
    init.limits.maxTransientVbSize = std::max(init.limits.maxTransientVbSize,
                                              kTransientVbBytes);
    init.limits.maxTransientIbSize = std::max(init.limits.maxTransientIbSize,
                                              kTransientIbBytes);

    uint32_t resetFlags = BGFX_RESET_VSYNC | BGFX_RESET_FLUSH_AFTER_RENDER;
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

    if (const bgfx::Caps* caps = bgfx::getCaps()) {
        std::fprintf(stderr,
            "[UILO] bgfx caps: maxDrawCalls=%u transientVB=%u KB transientIB=%u KB\n",
            caps->limits.maxDrawCalls,
            caps->limits.maxTransientVbSize / 1024u,
            caps->limits.maxTransientIbSize / 1024u);
    }

    bgfx::setDebug(BGFX_DEBUG_NONE);

    m_impl->ensureLayouts();
    if (!m_impl->initShaders()) return false;

    m_msaa        = msaa;
    m_initialised = true;
    return true;
}

bool Renderer::attach(SDL_Window* hostWindow, uint16_t baseView) {
    if (m_initialised) return true;

    m_window      = hostWindow;        // borrowed; not destroyed in shutdown()
    m_ownsContext = false;             // host owns SDL + bgfx + the frame loop
    m_impl->embedded = true;           // composite alpha-blends over the host image
    m_impl->setViewBase(baseView);     // rebase the pipeline views above the host's
    m_nextViewId  = baseView + 6;      // user framebuffers above UILO's pipeline

    // bgfx + window already exist; just build UILO's own GPU resources.
    m_impl->ensureLayouts();
    if (!m_impl->initShaders()) return false;

    m_initialised = true;
    return true;
}

void Renderer::shutdown() {
    if (!m_initialised) return;
    if (m_impl) m_impl->shutdownResources();   // UILO's own FBs/shaders, both modes
    if (m_ownsContext) {
        bgfx::shutdown();
        if (m_window) SDL_DestroyWindow(m_window);
        SDL_Quit();
    }
    m_window = nullptr; // borrowed in attach mode; just drop the reference
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

RendererStats Renderer::getStats() const {
    RendererStats out;
    const bgfx::Stats* s = bgfx::getStats();
    if (!s) return out;
    out.numDraw     = s->numDraw;
    out.numVertices = 0; // bgfx::Stats doesn't break out vertex counts in the public struct
    const double toMs = 1000.0 / (double)s->cpuTimerFreq;
    out.cpuTimeMs   = double(s->cpuTimeEnd - s->cpuTimeBegin) * toMs;
    const double gpuToMs = s->gpuTimerFreq ? 1000.0 / (double)s->gpuTimerFreq : 0.0;
    out.gpuTimeMs   = double(s->gpuTimeEnd - s->gpuTimeBegin) * gpuToMs;
    return out;
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
    // bgfx::reset can invalidate view->framebuffer bindings on some
    // backends. Force ensureSceneFramebuffers() to rebuild + re-bind every
    // reserved view on the next beginFrame by clearing the cached size.
    m_impl->fbWidth  = 0;
    m_impl->fbHeight = 0;
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
            case CursorType::Crosshair:      sdlType = SDL_SYSTEM_CURSOR_CROSSHAIR; break;
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
        if (m_ownsContext) bgfx::reset(sz.x, sz.y, m_resetFlags); // host owns reset when embedded
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

    // Predict whether this frame will use glass based on what we saw
    // last frame. When bypassing, view 0 renders straight to the
    // backbuffer and the endFrame pipeline (blur+replay+composite) is
    // skipped entirely.
    m_impl->bypassSceneFb     = !m_impl->hadGlassLastFrame;
    m_impl->hadGlassThisFrame = false;

    // Embedded: never bypass to the backbuffer (that would clear the host's
    // scene). Always render to sceneFB, then composite over the host image.
    if (!m_ownsContext) m_impl->bypassSceneFb = false;

    // Scene view → sceneFB (set every frame in case bgfx reset clobbered it).
    const uint16_t sceneView = m_impl->kSceneViewId;
    if (m_impl->bypassSceneFb) {
        bgfx::setViewFrameBuffer(sceneView, BGFX_INVALID_HANDLE);
    } else {
        bgfx::setViewFrameBuffer(sceneView, m_impl->sceneFB);
    }
    bgfx::setViewRect(sceneView, 0, 0, (uint16_t)sz.x, (uint16_t)sz.y);
    // Transparent clear when embedded so the UI composites over the host scene.
    const uint32_t sceneClear = m_ownsContext ? 0x000000ff : 0x00000000;
    bgfx::setViewClear(sceneView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, sceneClear, 1.f, 0);
    bgfx::setViewMode(sceneView, bgfx::ViewMode::Sequential);
    submitOrtho(sceneView, sz);
    bgfx::touch(sceneView);

    // Defensively clear any rotation left set by user code from the last
    // frame so internal/system draws (composite, blur, etc.) never inherit.
    clearRotation();
    m_impl->deferredGlass.clear();
    // Reset clip-uniform dedup so the first draw of the frame always
    // pushes its uniforms (bgfx uniform state isn't guaranteed to
    // persist across bgfx::frame()).
    m_impl->lastClipValid = false;
    // Defensive reset: clip stacks should be balanced each frame.
    m_impl->scissorTop = 0;
    m_impl->roundClipTop = 0;
    m_impl->scissorOverflowDepth = 0;
    m_impl->roundClipOverflowDepth = 0;
}

void Renderer::endFrame() {
    // Flush any rects still sitting in the solid-rect batch from the
    // last user draw call before kicking off internal passes.
    m_impl->flushSolidBatch();

    // Carry the per-frame glass-presence flag into the next frame's
    // prediction. Done before the early-out below so the next frame
    // correctly switches back to the FB pipeline when glass appeared.
    m_impl->hadGlassLastFrame = m_impl->hadGlassThisFrame;

    if (m_impl->bypassSceneFb) {
        // Scene was rendered directly to backbuffer; nothing else to do.
        m_impl->deferredGlass.clear();
        if (m_ownsContext) bgfx::frame(); // host presents when embedded
        // Framerate cap (unchanged from the FB path below).
        if (m_frameInterval > 0.0) {
            using clock = std::chrono::steady_clock;
            const auto now    = clock::now();
            const auto nowNs  = (uint64_t)std::chrono::duration_cast<
                                    std::chrono::nanoseconds>(now.time_since_epoch()).count();
            const uint64_t intervalNs = (uint64_t)(m_frameInterval * 1e9);
            if (m_nextFrameTick == 0 || nowNs > m_nextFrameTick + intervalNs) {
                m_nextFrameTick = nowNs + intervalNs;
            } else {
                if (nowNs < m_nextFrameTick) {
                    const uint64_t waitNs = m_nextFrameTick - nowNs;
                    constexpr uint64_t spinMarginNs = 500'000;
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
        return;
    }

    // After the scene has been submitted to sceneFB (without any glass
    // elements — those were deferred), run the blur ladder so glass
    // samples a glass-free backdrop, then replay the deferred glass
    // draws into sceneFB, then composite.
    Vec2u sz = getSize();
    // Skip the (expensive) blur ladder + glass replay entirely when no
    // glass material drew this frame. The blur target's contents become
    // stale but nothing samples them this frame, so it doesn't matter.
    const bool anyGlass = !m_impl->deferredGlass.empty();
    if (anyGlass) {
        m_impl->runBlurPasses(sz.x, sz.y);
    }

    if (!m_impl->deferredGlass.empty()) {
        // kGlassBgViewId was configured in beginFrame; push it so
        // drawGlass submits there during replay. It executes BEFORE
        // kGlassChildViewId (where child elements were submitted), so
        // glass backgrounds end up beneath their children in sceneFB.
        m_viewStack[m_viewStackTop++] = { m_impl->kGlassBgViewId };
        m_impl->replayingGlass = true;
        for (const auto& d : m_impl->deferredGlass) {
            // Re-establish the scissor that was active at capture time.
            // (Push then pop around each replay so they don't accumulate.)
            if (d.hasScissor) {
                m_impl->scissorStack[m_impl->scissorTop++] =
                    { d.sx, d.sy, d.sw, d.sh };
            }
            drawGlass(d.dst, d.mat, d.baseColor);
            if (d.hasScissor) {
                --m_impl->scissorTop;
            }
        }
        m_impl->replayingGlass = false;
        m_impl->deferredGlass.clear();
        if (m_viewStackTop > 0) --m_viewStackTop;
    }

    m_impl->compositeSceneToBackbuffer(sz.x, sz.y,
                                       m_impl->texLayout,
                                       m_impl->texProgram);

    if (m_ownsContext) bgfx::frame(); // host presents when embedded
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
    return m_impl->kSceneViewId; // rebased scene view (was hardcoded 0)
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
    m_impl->flushSolidBatch();
    assert(m_viewStackTop < kMaxViewStack);
    m_viewStack[m_viewStackTop++] = { fb.viewId };
    bgfx::setViewRect(fb.viewId, 0, 0, (uint16_t)fb.size.x, (uint16_t)fb.size.y);
    submitOrtho(fb.viewId, fb.size);
}

void Renderer::popFrameBuffer() {
    m_impl->flushSolidBatch();
    if (m_viewStackTop > 0) --m_viewStackTop;
}

void Renderer::beginGlassSubtree() {
    m_impl->flushSolidBatch();
    assert(m_viewStackTop < kMaxViewStack);
    m_viewStack[m_viewStackTop++] = { m_impl->kGlassChildViewId };
}

void Renderer::endGlassSubtree() {
    m_impl->flushSolidBatch();
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
    impl.flushSolidBatch();
    if (impl.scissorTop >= Impl::kMaxScissor) {
        ++impl.scissorOverflowDepth;
        return;
    }
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

    // Quantize to integer pixel bounds conservatively: floor min edges and
    // ceil max edges so clipping remains symmetric and stable as UI scale
    // introduces fractional coordinates.
    const float qx0f = std::floor(b.position.x);
    const float qy0f = std::floor(b.position.y);
    const float qx1f = std::ceil(b.position.x + b.size.x);
    const float qy1f = std::ceil(b.position.y + b.size.y);

    const float x0f = std::max(0.f, qx0f);
    const float y0f = std::max(0.f, qy0f);
    const float x1f = std::max(x0f, qx1f);
    const float y1f = std::max(y0f, qy1f);

    const uint16_t x0 = (uint16_t)x0f;
    const uint16_t y0 = (uint16_t)y0f;
    const uint16_t w  = (uint16_t)std::max(0.f, x1f - x0f);
    const uint16_t h  = (uint16_t)std::max(0.f, y1f - y0f);

    impl.scissorStack[impl.scissorTop++] = {
        x0,
        y0,
        w,
        h
    };
}

void Renderer::popScissor() {
    m_impl->flushSolidBatch();
    if (m_impl->scissorOverflowDepth > 0) {
        --m_impl->scissorOverflowDepth;
        return;
    }
    if (m_impl->scissorTop > 0) --m_impl->scissorTop;
}

void Renderer::pushRoundClip(Rectf b, float radius) {
    auto& impl = *m_impl;
    impl.flushSolidBatch();
    pushScissor(b);
    if (impl.roundClipTop >= Impl::kMaxRoundClip) {
        ++impl.roundClipOverflowDepth;
        return;
    }
    float r = std::max(0.f, radius);
    if (r <= 0.f) {
        // Plain rectangle: still record an "off" entry so pop balances
        // and (when there's already a rounded parent clip) the parent
        // clip still applies to children that don't add their own.
        if (impl.roundClipTop == 0) {
            impl.roundClipStack[impl.roundClipTop++] = {0.f, 0.f, 0.f, 0.f, -1.f};
        } else {
            // Inherit parent's clip verbatim so SDF mask doesn't change.
            impl.roundClipStack[impl.roundClipTop] =
                impl.roundClipStack[impl.roundClipTop - 1];
            impl.roundClipTop++;
        }
        return;
    }
    // Snap rounded clip bounds to pixel edges to keep AA and corner shape
    // visually consistent across scales.
    const float x0 = std::floor(b.position.x);
    const float y0 = std::floor(b.position.y);
    const float x1 = std::ceil(b.position.x + b.size.x);
    const float y1 = std::ceil(b.position.y + b.size.y);

    float halfW = std::max(0.f, (x1 - x0) * 0.5f);
    float halfH = std::max(0.f, (y1 - y0) * 0.5f);
    float cx    = x0 + halfW;
    float cy    = y0 + halfH;
    // Do NOT intersect the child's rounded-rect SDF with the parent's
    // rect. The SDF defines the *shape* of this element (circle, pill,
    // rounded button, etc.); shrinking halfW/halfH to the visible
    // intersection would force the radius to clamp against min(halfW,
    // halfH) and visually squash the corners as the element overflows
    // the parent. Rectangular cropping is already handled by the
    // scissor pushed above; soft cropping by the parent's rounded edge
    // is an acceptable trade-off we skip here.
    r = std::min(r, std::min(halfW, halfH));
    impl.roundClipStack[impl.roundClipTop++] = {cx, cy, halfW, halfH, r};
}

void Renderer::popRoundClip() {
    auto& impl = *m_impl;
    impl.flushSolidBatch();
    if (impl.roundClipOverflowDepth > 0) {
        --impl.roundClipOverflowDepth;
    } else if (impl.roundClipTop > 0) {
        --impl.roundClipTop;
    }
    popScissor();
}

void Renderer::setRotation(float degrees, Vec2f pivot) {
    m_impl->flushSolidBatch();
    auto& r = m_impl->rotation;
    r.pivotX   = pivot.x;
    r.pivotY   = pivot.y;
    r.angleDeg = degrees;
    const float rad = degrees * (3.14159265f / 180.f);
    r.cosA = std::cos(rad);
    r.sinA = std::sin(rad);
    r.enabled = std::abs(r.sinA) > 1e-6f || std::abs(r.cosA - 1.f) > 1e-6f;
}

void Renderer::rotate(float deltaDegrees) {
    m_impl->flushSolidBatch();
    auto& r = m_impl->rotation;
    r.angleDeg += deltaDegrees;
    const float rad = r.angleDeg * (3.14159265f / 180.f);
    r.cosA = std::cos(rad);
    r.sinA = std::sin(rad);
    r.enabled = std::abs(r.sinA) > 1e-6f || std::abs(r.cosA - 1.f) > 1e-6f;
}

void Renderer::clearRotation() {
    m_impl->flushSolidBatch();
    auto& r = m_impl->rotation;
    r.angleDeg = 0.f;
    r.cosA = 1.f;
    r.sinA = 0.f;
    r.enabled = false;
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
    m_impl->flushSolidBatch();
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
inline void applyRoundClipInner(Renderer::Impl& impl) {
    // Walk the clip stack from the top down and collect the two
    // top-most rounded entries (radius > 0). The first becomes the
    // "inner" SDF (the shape being drawn, e.g. a button's own rounded
    // body); the second becomes the "outer" SDF (the enclosing rounded
    // ancestor, e.g. the parent panel). Both are applied in the
    // fragment shader, so a child element stays its own shape AND
    // gets cropped by the parent's rounded corners.
    float rect[4]    = {0.f, 0.f, 0.f, 0.f};
    float params[4]  = {0.f, 0.f, 0.f, 0.f};
    float rect2[4]   = {0.f, 0.f, 0.f, 0.f};
    float params2[4] = {0.f, 0.f, 0.f, 0.f};
    int picked = 0;
    for (int i = impl.roundClipTop - 1; i >= 0 && picked < 2; --i) {
        const auto& c = impl.roundClipStack[i];
        if (c.radius <= 0.f) continue;
        if (picked == 0) {
            rect[0] = c.cx; rect[1] = c.cy;
            rect[2] = c.halfW; rect[3] = c.halfH;
            params[0] = c.radius; params[1] = 1.f;
        } else {
            rect2[0] = c.cx; rect2[1] = c.cy;
            rect2[2] = c.halfW; rect2[3] = c.halfH;
            params2[0] = c.radius; params2[1] = 1.f;
        }
        ++picked;
    }
    // Dedup: bgfx setUniform on a Vec4 is cheap but not free, and the
    // overwhelming majority of consecutive draws share the same clip
    // (sibling children inside the same container). Skip the push when
    // nothing changed.
    const bool same = impl.lastClipValid
        && rect[0]    == impl.lastClipRect[0]    && rect[1]    == impl.lastClipRect[1]
        && rect[2]    == impl.lastClipRect[2]    && rect[3]    == impl.lastClipRect[3]
        && params[0]  == impl.lastClipParams[0]  && params[1]  == impl.lastClipParams[1]
        && rect2[0]   == impl.lastClipRect2[0]   && rect2[1]   == impl.lastClipRect2[1]
        && rect2[2]   == impl.lastClipRect2[2]   && rect2[3]   == impl.lastClipRect2[3]
        && params2[0] == impl.lastClipParams2[0] && params2[1] == impl.lastClipParams2[1];
    if (same) return;
    if (bgfx::isValid(impl.u_clipRect))    bgfx::setUniform(impl.u_clipRect,    rect);
    if (bgfx::isValid(impl.u_clipParams))  bgfx::setUniform(impl.u_clipParams,  params);
    if (bgfx::isValid(impl.u_clipRect2))   bgfx::setUniform(impl.u_clipRect2,   rect2);
    if (bgfx::isValid(impl.u_clipParams2)) bgfx::setUniform(impl.u_clipParams2, params2);
    impl.lastClipRect[0]    = rect[0];    impl.lastClipRect[1]    = rect[1];
    impl.lastClipRect[2]    = rect[2];    impl.lastClipRect[3]    = rect[3];
    impl.lastClipParams[0]  = params[0];  impl.lastClipParams[1]  = params[1];
    impl.lastClipParams[2]  = params[2];  impl.lastClipParams[3]  = params[3];
    impl.lastClipRect2[0]   = rect2[0];   impl.lastClipRect2[1]   = rect2[1];
    impl.lastClipRect2[2]   = rect2[2];   impl.lastClipRect2[3]   = rect2[3];
    impl.lastClipParams2[0] = params2[0]; impl.lastClipParams2[1] = params2[1];
    impl.lastClipParams2[2] = params2[2]; impl.lastClipParams2[3] = params2[3];
    impl.lastClipValid = true;
}
inline void applyScissor(Renderer::Impl& impl) {
    if (impl.scissorTop > 0) {
        auto& sc = impl.scissorStack[impl.scissorTop - 1];
        bgfx::setScissor(sc.x, sc.y, sc.w, sc.h);
    }
    applyRoundClipInner(impl);
}
inline void applyRoundClip(Renderer::Impl& impl) {
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

    const uint16_t view = currentViewId();
    if (impl.solidBatchView != view && !impl.solidBatchVerts.empty()) {
        impl.flushSolidBatch();
    }
    impl.solidBatchView = view;

    uint32_t col = packColor(r.fillColor);
    float x = r.position.x, y = r.position.y;
    float w = r.size.x,     h = r.size.y;

    float x0 = x,     y0 = y;
    float x1 = x + w, y1 = y;
    float x2 = x + w, y2 = y + h;
    float x3 = x,     y3 = y + h;
    impl.rotPt(x0, y0); impl.rotPt(x1, y1);
    impl.rotPt(x2, y2); impl.rotPt(x3, y3);

    const uint16_t base = (uint16_t)impl.solidBatchVerts.size();
    // A frame can legitimately overflow uint16_t indices (65535 / 4 = 16383
    // rects). Flush before crossing the line.
    if (base + 4 > 65532) {
        impl.flushSolidBatch();
        impl.solidBatchView = view;
    }
    const uint16_t b2 = (uint16_t)impl.solidBatchVerts.size();
    impl.solidBatchVerts.push_back({x0, y0, col});
    impl.solidBatchVerts.push_back({x1, y1, col});
    impl.solidBatchVerts.push_back({x2, y2, col});
    impl.solidBatchVerts.push_back({x3, y3, col});
    impl.solidBatchIdx.push_back(b2 + 0);
    impl.solidBatchIdx.push_back(b2 + 1);
    impl.solidBatchIdx.push_back(b2 + 2);
    impl.solidBatchIdx.push_back(b2 + 0);
    impl.solidBatchIdx.push_back(b2 + 2);
    impl.solidBatchIdx.push_back(b2 + 3);

    if (r.outlineThickness > 0.f && r.outlineColor.a > 0) {
        float t = r.outlineThickness;
        Rect top    { r.position,        {w, t}, r.outlineColor };
        Rect bottom { {x, y + h - t},    {w, t}, r.outlineColor };
        Rect left   { r.position,        {t, h}, r.outlineColor };
        Rect right  { {x + w - t, y},    {t, h}, r.outlineColor };
        draw(top); draw(bottom); draw(left); draw(right);
    }
}

void Renderer::Impl::flushSolidBatch() {
    if (solidBatchVerts.empty() || solidBatchView == UINT16_MAX) {
        solidBatchVerts.clear();
        solidBatchIdx.clear();
        return;
    }
    if (!bgfx::isValid(solidProgram)) {
        solidBatchVerts.clear();
        solidBatchIdx.clear();
        solidBatchView = UINT16_MAX;
        return;
    }
    const uint32_t numV = (uint32_t)solidBatchVerts.size();
    const uint32_t numI = (uint32_t)solidBatchIdx.size();
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (bgfx::allocTransientBuffers(&tvb, solidLayout, numV, &tib, numI)) {
        std::memcpy(tvb.data, solidBatchVerts.data(), numV * sizeof(PosColorVertex));
        std::memcpy(tib.data, solidBatchIdx.data(),   numI * sizeof(uint16_t));
        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setIndexBuffer(&tib);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                       BGFX_STATE_BLEND_ALPHA);
        // Apply the scissor/round-clip snapshot in effect right now; the
        // batch is always flushed before any state change so this matches
        // every rect in the batch.
        applyScissor(*this);
        bgfx::submit(solidBatchView, solidProgram);
    }
    solidBatchVerts.clear();
    solidBatchIdx.clear();
    solidBatchView = UINT16_MAX;
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
        float x0 = x,     y0 = y;
        float x1 = x + w, y1 = y;
        float x2 = x + w, y2 = y + h;
        float x3 = x,     y3 = y + h;
        impl.rotPt(x0, y0); impl.rotPt(x1, y1);
        impl.rotPt(x2, y2); impl.rotPt(x3, y3);
        PosColorVertex verts[4] = {
            {x0, y0, col},
            {x1, y1, col},
            {x2, y2, col},
            {x3, y3, col},
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
    impl.flushSolidBatch();
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;
    if (c.radius <= 0.f || c.fillColor.a == 0) return;

    // Render as a rounded-rect with radius == half-size — the fragment
    // shader SDF gives proper sub-pixel AA, matching Button/Dropdown.
    // Inflate the quad by 1px so the AA falloff has room outside the
    // disc's geometric bounds (the SDF eats ~1px of edge).
    const float r   = c.radius;
    const float pad = 1.f;
    const float x   = c.center.x - r - pad;
    const float y   = c.center.y - r - pad;
    const float w   = (r + pad) * 2.f;
    const float h   = (r + pad) * 2.f;

    pushRoundClip({{c.center.x - r, c.center.y - r}, {r * 2.f, r * 2.f}}, r);

    uint32_t col = packColor(c.fillColor);
    float x0 = x,     y0 = y;
    float x1 = x + w, y1 = y;
    float x2 = x + w, y2 = y + h;
    float x3 = x,     y3 = y + h;
    impl.rotPt(x0, y0); impl.rotPt(x1, y1);
    impl.rotPt(x2, y2); impl.rotPt(x3, y3);
    PosColorVertex verts[4] = {
        {x0, y0, col},
        {x1, y1, col},
        {x2, y2, col},
        {x3, y3, col},
    };
    uint16_t idx[6] = {0,1,2, 0,2,3};

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.solidLayout, 4, &tib, 6)) {
        popRoundClip();
        return;
    }
    std::memcpy(tvb.data, verts, sizeof(verts));
    std::memcpy(tib.data, idx,   sizeof(idx));
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.solidProgram);

    popRoundClip();
}

void Renderer::draw(const Triangle& t) {
    auto& impl = *m_impl;
    impl.flushSolidBatch();
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;

    uint32_t col = packColor(t.fillColor);
    float ax = t.a.x, ay = t.a.y;
    float bx = t.b.x, by = t.b.y;
    float cx = t.c.x, cy = t.c.y;
    impl.rotPt(ax, ay);
    impl.rotPt(bx, by);
    impl.rotPt(cx, cy);
    PosColorVertex verts[3] = {
        {ax, ay, col},
        {bx, by, col},
        {cx, cy, col},
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
    impl.flushSolidBatch();
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;

    float dx = l.end.x - l.start.x;
    float dy = l.end.y - l.start.y;
    float len = std::sqrt(dx*dx + dy*dy);
    if (len < 0.001f) return;

    // Unit normal (perp to line direction).
    const float ux = -dy / len;
    const float uy =  dx / len;
    const float half = l.thickness * 0.5f;
    const float pad  = 1.f; // skirt width in pixels for edge AA

    // Six vertices per line endpoint: outer skirt | edge | inner edge twice
    // simplifies to 4 rows along the normal at offsets:
    //   +half+pad (a=0), +half (a=1), -half (a=1), -half-pad (a=0)
    const float offs[4]   = {  half + pad,  half, -half, -half - pad };
    const uint8_t alphas[4] = { 0, 255, 255, 0 };

    const uint8_t baseA = l.color.a;
    float vx[8], vy[8]; // 2 endpoints * 4 rows
    uint32_t vc[8];
    for (int ep = 0; ep < 2; ++ep) {
        const float bx = (ep == 0) ? l.start.x : l.end.x;
        const float by = (ep == 0) ? l.start.y : l.end.y;
        for (int r = 0; r < 4; ++r) {
            float x = bx + ux * offs[r];
            float y = by + uy * offs[r];
            impl.rotPt(x, y);
            vx[ep*4 + r] = x;
            vy[ep*4 + r] = y;
            uint8_t a = (uint8_t)((uint16_t)baseA * (uint16_t)alphas[r] / 255);
            vc[ep*4 + r] = packColor(Color{l.color.r, l.color.g, l.color.b, a});
        }
    }

    PosColorVertex verts[8];
    for (int i = 0; i < 8; ++i) verts[i] = { vx[i], vy[i], vc[i] };

    // 3 quads (skirt, body, skirt) between the two endpoints.
    uint16_t idx[18];
    int ii = 0;
    for (int r = 0; r < 3; ++r) {
        const uint16_t v00 = (uint16_t)(0*4 + r);
        const uint16_t v01 = (uint16_t)(0*4 + r + 1);
        const uint16_t v10 = (uint16_t)(1*4 + r);
        const uint16_t v11 = (uint16_t)(1*4 + r + 1);
        idx[ii++] = v00; idx[ii++] = v01; idx[ii++] = v11;
        idx[ii++] = v00; idx[ii++] = v11; idx[ii++] = v10;
    }

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.solidLayout, 8, &tib, 18))
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
    impl.flushSolidBatch();
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
            float x0 = l.start.x + nx, y0 = l.start.y + ny;
            float x1 = l.start.x - nx, y1 = l.start.y - ny;
            float x2 = l.end.x   - nx, y2 = l.end.y   - ny;
            float x3 = l.end.x   + nx, y3 = l.end.y   + ny;
            impl.rotPt(x0, y0); impl.rotPt(x1, y1);
            impl.rotPt(x2, y2); impl.rotPt(x3, y3);
            verts[vi+0] = {x0, y0, col};
            verts[vi+1] = {x1, y1, col};
            verts[vi+2] = {x2, y2, col};
            verts[vi+3] = {x3, y3, col};
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

void Renderer::drawArc(Vec2f center, float innerR, float outerR,
                       float startDeg, float endDeg, Color color, int segments) {
    auto& impl = *m_impl;
    impl.flushSolidBatch();
    if (!bgfx::isValid(impl.solidProgram) || scissorEmpty(impl)) return;
    if (color.a == 0) return;

    if (innerR > outerR) std::swap(innerR, outerR);
    if (outerR <= 0.f) return;
    if (innerR < 0.f) innerR = 0.f;

    int segs = std::max(1, segments);
    // We emit (segs + 2 angular skirt) slices x 4 radial rows. Cap so
    // vertex / index counts stay in 16-bit range. Per slice = 4 verts,
    // 6 quads (between adjacent slices) x 6 indices = 18 indices.
    constexpr int kMaxSlices = (65535 / 6);
    if (segs + 2 > kMaxSlices) segs = kMaxSlices - 2;

    const float startRad = startDeg * kDeg2Rad;
    const float endRad   = endDeg   * kDeg2Rad;
    const float step     = (endRad - startRad) / (float)segs;

    // Angular skirt width = ~1 pixel of arc length at outerR.
    const float angSkirt = (outerR > 0.5f) ? (1.f / outerR) : 0.f;
    const float startSk  = startRad - (step >= 0.f ? angSkirt : -angSkirt);
    const float endSk    = endRad   + (step >= 0.f ? angSkirt : -angSkirt);

    // Radial rows: outer skirt | outer | inner | inner skirt.
    const float innerSk = std::max(0.f, innerR - 1.f);
    const float outerSk = outerR + 1.f;
    const float radii[4] = { outerSk, outerR, innerR, innerSk };
    // Alpha modulation per row (skirts fade to 0).
    const uint8_t alphaMul[4] = { 0, 255, 255, 0 };
    // If innerR <= 0 we draw a wedge; pull the inner skirt in and keep it
    // at alpha 1 (no inner hole, no AA needed).
    const bool solidCore = innerR <= 0.f;

    const int sliceCount = segs + 3; // [-1, 0..segs, segs+1]
    const uint32_t nV = (uint32_t)sliceCount * 4;
    // Quads per slice gap: 3 normally, or 2 if solidCore (skip inner-skirt row).
    const int rowsPerGap = solidCore ? 2 : 3;
    const uint32_t nI = (uint32_t)(sliceCount - 1) * rowsPerGap * 6;

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.solidLayout, nV, &tib, nI))
        return;

    auto* verts = reinterpret_cast<PosColorVertex*>(tvb.data);
    auto* idx   = reinterpret_cast<uint16_t*>(tib.data);

    auto sliceAngle = [&](int s) -> float {
        // s = 0       -> startSk
        // s = 1       -> startRad
        // s = segs+1  -> endRad
        // s = segs+2  -> endSk
        if (s == 0)              return startSk;
        if (s == sliceCount - 1) return endSk;
        return startRad + step * (float)(s - 1);
    };
    auto sliceAngularAlpha = [&](int s) -> uint8_t {
        return (s == 0 || s == sliceCount - 1) ? 0 : 255;
    };

    for (int s = 0; s < sliceCount; ++s) {
        const float a   = sliceAngle(s);
        const float ca  = std::cos(a);
        const float sa  = std::sin(a);
        const uint8_t am = sliceAngularAlpha(s);
        for (int r = 0; r < 4; ++r) {
            float px = center.x + radii[r] * ca;
            float py = center.y + radii[r] * sa;
            impl.rotPt(px, py);
            // Combine radial-skirt alpha with angular-skirt alpha
            // (multiplicative). Solid-core forces inner skirt alpha to 1.
            uint8_t ra = (solidCore && r == 3) ? 255 : alphaMul[r];
            uint16_t a16 = (uint16_t)ra * (uint16_t)am;
            uint8_t finalA = (uint8_t)((a16 + 127) / 255);
            Color cc{color.r, color.g, color.b,
                     (uint8_t)((uint16_t)color.a * (uint16_t)finalA / 255)};
            verts[s*4 + r] = { px, py, packColor(cc) };
        }
    }

    uint32_t ii = 0;
    for (int s = 0; s < sliceCount - 1; ++s) {
        for (int r = 0; r < rowsPerGap; ++r) {
            const uint16_t v00 = (uint16_t)(s*4 + r);
            const uint16_t v01 = (uint16_t)(s*4 + r + 1);
            const uint16_t v10 = (uint16_t)((s+1)*4 + r);
            const uint16_t v11 = (uint16_t)((s+1)*4 + r + 1);
            idx[ii++] = v00;
            idx[ii++] = v01;
            idx[ii++] = v11;
            idx[ii++] = v00;
            idx[ii++] = v11;
            idx[ii++] = v10;
        }
    }

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.solidProgram);
}

} // namespace uilo
