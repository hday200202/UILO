#include "RendererImpl.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <string_view>
#include <thread>

/* The UI program's SPIR-V, compiled and linked in by valo_embed_shaders(uilo)
   in CMakeLists.txt. uilo is a static library, so the embedded shaders are
   registered explicitly rather than left to static initialisers the linker
   would drop. */
VALO_DECLARE_EMBEDDED_SHADERS(uilo)

namespace uilo {

namespace {

/* The program's two halves, as registered by the shader headers. Matched by
   suffix, so this is the same key on both backends and from any build dir. */
constexpr const char* kUiVertexShader   = "renderer/shaders/ui.vert.hpp";
constexpr const char* kUiFragmentShader = "renderer/shaders/ui.frag.hpp";

/*
    toValoColor(Color c):
    - Params:   Color c
    - Returns:  valo::Color
    - Desc:     A UILO colour as VALO's 0..1 floats, for layer clears.
*/
valo::Color toValoColor(Color c) {
    return valo::Color{c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
}

/*
    sleepUntilNextFrame(double interval, uint64_t& nextTick):
    - Params:   double interval -- seconds per frame; uint64_t& nextTick --
                steady_clock ns of the next deadline, 0 to re-anchor
    - Returns:  none
    - Desc:     The frame-rate cap. Sleeps most of the remaining time and spins
                the last half millisecond, so scheduler latency does not
                overshoot the deadline; re-anchors when the loop has fallen more
                than a whole frame behind rather than trying to catch up.
*/
void sleepUntilNextFrame(double interval, uint64_t& nextTick) {
    if (interval <= 0.0) return;
    using clock = std::chrono::steady_clock;
    const auto nowNs = [] {
        return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
            clock::now().time_since_epoch()).count();
    };
    const uint64_t now        = nowNs();
    const uint64_t intervalNs = (uint64_t)(interval * 1e9);
    if (nextTick == 0 || now > nextTick + intervalNs) {
        nextTick = now + intervalNs;
        return;
    }
    if (now < nextTick) {
        const uint64_t waitNs = nextTick - now;
        constexpr uint64_t spinMarginNs = 500'000;   /* 0.5 ms */
        if (waitNs > spinMarginNs)
            std::this_thread::sleep_for(std::chrono::nanoseconds(waitNs - spinMarginNs));
        while (nowNs() < nextTick) std::this_thread::yield();
    }
    nextTick += intervalNs;
}

} // anon

/*
    createResources():
    - Params:   none
    - Returns:  bool
    - Desc:     Creates what every frame draws through: the UI program and the
                arena's vertex and index buffers. The buffers start with one
                placeholder element because VALO creates a buffer from data;
                the first frame replaces it.
*/
bool Renderer::Impl::createResources() {
    layout = valo::VtxLayout({ valo::VtxAttrib::Vec2f,
                               valo::VtxAttrib::Vec4f,
                               valo::VtxAttrib::Vec2f });

    const UiVertex seedVertex{};
    const uint32_t seedIndex = 0;
    vb = valo.createVertexBuffer(&seedVertex, 1, layout);
    ib = valo.createIndexBuffer(&seedIndex, 1);

    VALO_REGISTER_EMBEDDED_SHADERS(uilo);
    program = valo.createProgram(kUiVertexShader, kUiFragmentShader);
    if (program == valo::DEFAULT_PROGRAM) {
        std::fprintf(stderr, "[UILO] failed to create the UI shader program\n");
        return false;
    }
    return vb != valo::EMPTY_BUFFER && ib != valo::EMPTY_BUFFER;
}

/*
    shutdownResources():
    - Params:   none
    - Returns:  none
    - Desc:     Releases every GPU resource the renderer created -- textures,
                font atlases, framebuffer layers, the program and the arena --
                then VALO itself and the window.
*/
void Renderer::Impl::shutdownResources() {
    for (auto& kv : textureCache)
        if (kv.second.valid()) valo.destroyTexture(kv.second.handle);
    textureCache.clear();

    for (auto& rec : fonts)
        for (auto& kv : rec.sizes)
            if (kv.second.atlas != valo::EMPTY_TEXTURE) valo.destroyTexture(kv.second.atlas);
    fonts.clear();
    fontByPath.clear();

    if (program != valo::DEFAULT_PROGRAM) valo.destroyProgram(program);
    if (vb != valo::EMPTY_BUFFER)        valo.destroyVertexBuffer(vb);
    if (ib != valo::EMPTY_BUFFER)        valo.destroyIndexBuffer(ib);
    program = valo::DEFAULT_PROGRAM;
    vb = valo::EMPTY_BUFFER;
    ib = valo::EMPTY_BUFFER;

    if (valoReady) valo.shutdown();
    valoReady = false;
    window.reset();
}

/*
    applyLayerOrder():
    - Params:   none
    - Returns:  none
    - Desc:     Framebuffer layers run before the window's, so a framebuffer
                drawn into the scene has already been rendered that frame.
*/
void Renderer::Impl::applyLayerOrder() {
    std::vector<valo::LayerID> order = fbLayers;
    order.push_back(valo::VALO::BASE_LAYER);
    valo.setLayerOrder(order);
}

/*
    refreshClipCache():
    - Params:   none
    - Returns:  none
    - Desc:     Walk the clip stack from the top down and collect the two top-
                most rounded entries (radius > 0). The first becomes the "inner"
                SDF (the shape being drawn, e.g. a button's own rounded body);
                the second becomes the "outer" SDF (the enclosing rounded
                ancestor, e.g. the parent panel). Both are applied in the
                fragment shader, so a child element stays its own shape AND gets
                cropped by the parent's rounded corners.
*/
void Renderer::Impl::refreshClipCache() {
    if (curClipVersion == clipVersion) return;
    for (int i = 0; i < 4; ++i) {
        curClipRect[i]    = 0.f;
        curClipParams[i]  = 0.f;
        curClipRect2[i]   = 0.f;
        curClipParams2[i] = 0.f;
    }
    int picked = 0;
    for (int i = roundClipTop - 1; i >= 0 && picked < 2; --i) {
        const auto& c = roundClipStack[i];
        if (c.radius <= 0.f) continue;
        if (picked == 0) {
            curClipRect[0] = c.cx;    curClipRect[1] = c.cy;
            curClipRect[2] = c.halfW; curClipRect[3] = c.halfH;
            curClipParams[0] = c.radius; curClipParams[1] = 1.f;
        } else {
            curClipRect2[0] = c.cx;    curClipRect2[1] = c.cy;
            curClipRect2[2] = c.halfW; curClipRect2[3] = c.halfH;
            curClipParams2[0] = c.radius; curClipParams2[1] = 1.f;
        }
        ++picked;
    }
    curClipVersion = clipVersion;
}

/*
    beginShape(uint32_t mode, valo::TextureID tex, const float imgFlags[4]):
    - Params:   uint32_t mode, valo::TextureID tex, const float imgFlags[4]
    - Returns:  bool -- false when the current scissor is empty
    - Desc:     Makes sure the batch about to receive a shape's geometry was
                opened under exactly this shape's draw state -- target, shader
                mode, texture, image flags, both rounded clips and the scissor.
                Anything different submits the batch so far and starts a new
                one, so draw order is always submission order.
*/
bool Renderer::Impl::beginShape(uint32_t mode, valo::TextureID tex, const float imgFlags[4]) {
    /* A minimised window has nothing to draw into, and VALO only drains its
       queues when a frame runs, so nothing may be queued at all. */
    if (frameSize.x == 0 || frameSize.y == 0) return false;
    if (scissorEmpty()) return false;
    refreshClipCache();

    UiUniforms u{};
    u.clipRect    = valo::Vec4f{curClipRect[0],    curClipRect[1],    curClipRect[2],    curClipRect[3]};
    u.clipParams  = valo::Vec4f{curClipParams[0],  curClipParams[1],  curClipParams[2],  curClipParams[3]};
    u.clipRect2   = valo::Vec4f{curClipRect2[0],   curClipRect2[1],   curClipRect2[2],   curClipRect2[3]};
    u.clipParams2 = valo::Vec4f{curClipParams2[0], curClipParams2[1], curClipParams2[2], curClipParams2[3]};
    if (imgFlags)
        u.imgFlags = valo::Vec4f{imgFlags[0], imgFlags[1], imgFlags[2], imgFlags[3]};
    else
        u.imgFlags = valo::Vec4f{0.f, 0.f, 0.f, 0.f};
    const Vec2u target = currentTargetSize(frameSize);
    u.viewport = valo::Vec2f{static_cast<float>(std::max(1u, target.x)),
                             static_cast<float>(std::max(1u, target.y))};
    u.tex  = tex;
    u.mode = mode;

    valo::Scissor sc{};
    if (scissorTop > 0) {
        const auto& s = scissorStack[scissorTop - 1];
        sc = valo::Scissor{ int32_t(s.x), int32_t(s.y), uint32_t(s.w), uint32_t(s.h) };
    }

    const valo::LayerID layer = currentLayer();
    const bool same = batch.open
        && batch.layer == layer
        && std::memcmp(&batch.uniforms, &u, sizeof(UiUniforms)) == 0
        && batch.scissor.x == sc.x && batch.scissor.y == sc.y
        && batch.scissor.width == sc.width && batch.scissor.height == sc.height;
    if (same) return true;

    flushBatch();
    batch.open       = true;
    batch.layer      = layer;
    batch.firstIndex = static_cast<uint32_t>(idx.size());
    batch.indexCount = 0;
    batch.uniforms   = u;
    batch.scissor    = sc;
    return true;
}

/*
    flushBatch():
    - Params:   none
    - Returns:  none
    - Desc:     Submits the open batch as one VALO command and closes it. The
                command names a range of the arena, which is uploaded at
                endFrame before VALO runs the frame's queue.
*/
void Renderer::Impl::flushBatch() {
    if (!batch.open) return;
    batch.open = false;
    if (batch.indexCount == 0 || program == valo::DEFAULT_PROGRAM) return;

    valo::RenderCommand cmd;
    cmd.verts       = vb;
    cmd.indices     = ib;
    cmd.program     = program;
    cmd.firstIndex  = batch.firstIndex;
    cmd.indexCount  = batch.indexCount;
    cmd.topology    = valo::Topology::Triangles;
    cmd.cullmode    = valo::CullMode::None;
    cmd.blendmode   = valo::BlendMode::Alpha;
    cmd.depthTest   = false;
    cmd.depthWrite  = false;
    cmd.scissor     = batch.scissor;
    cmd.setUniforms(batch.uniforms);
    valo.submit(cmd, batch.layer);
    ++submittedCommands;
}

/*
    Renderer():
    - Params:   none
    - Returns:  Renderer
    - Desc:     Creates an uninitialised renderer; init() opens the window.
*/
Renderer::Renderer() : m_impl(std::make_unique<Impl>()) {}

/*
    ~Renderer():
    - Params:   none
    - Returns:  n/a
    - Desc:     Shuts the renderer down, releasing the GPU resources, VALO and
                the window.
*/
Renderer::~Renderer() { shutdown(); }

/*
    init(uint32_t width, uint32_t height, const std::string& title, uint8_t msaa):
    - Params:   uint32_t width, uint32_t height, const std::string& title,
                uint8_t msaa
    - Returns:  bool
    - Desc:     Opens the window and brings VALO up on it, then builds the
                program and the arena. Vulkan is tried first where VALO was
                built with it, and the software rasteriser is the fallback, so a
                machine without a usable GPU still opens. UILO_RENDERER=vulkan |
                software | auto forces the choice. Reports false rather than
                throwing if every option fails.
*/
bool Renderer::init(uint32_t width, uint32_t height,
                    const std::string& title, uint8_t msaa) {
    if (m_initialised) return true;
    auto& impl = *m_impl;

    bool tryVulkan   = false;
    bool trySoftware = true;
#if defined(VALO_HAS_VULKAN)
    tryVulkan = true;
#endif
    if (const char* env = std::getenv("UILO_RENDERER")) {
        const std::string_view want{env};
        if (want == "vulkan") {
            trySoftware = false;
#if !defined(VALO_HAS_VULKAN)
            std::fprintf(stderr, "[UILO] UILO_RENDERER=vulkan, but VALO was built without Vulkan\n");
            trySoftware = true;
#endif
        } else if (want == "software") {
            tryVulkan = false;
        } else if (want != "auto") {
            std::fprintf(stderr, "[UILO] unknown UILO_RENDERER '%s' (ignored)\n", env);
        }
    }

    const bool validation = [] {
        const char* v = std::getenv("UILO_VALIDATION");
        return v && std::string_view{v} != "0";
    }();

    auto bringUp = [&](valo::Backend backend) -> bool {
        valo::WinFlags flags = valo::WinFlags::Resize | valo::WinFlags::HiDPI;
        if (backend == valo::Backend::Vulkan) flags |= valo::WinFlags::Vulkan;

        /* respectOSScale: the size is in window points, as SDL_CreateWindow
           takes it, so the window comes out the same size it always has. */
        impl.window = std::make_unique<valo::Window>(title, valo::Vec2u{width, height}, flags, true);
        if (!impl.window->isOpen()) {
            impl.window.reset();
            return false;
        }

        valo::RenderConfig cfg;
        cfg.vsync          = impl.vsync;
        cfg.backend        = backend;
        cfg.validation     = validation;
        cfg.framesInFlight = 2;
        /* The window's multisampling, as bgfx's backbuffer had it: clamped to
           what the device can do. Software ignores it. */
        cfg.msaa           = msaa;
        if (!impl.valo.init(*impl.window, cfg)) {
            impl.valo.shutdown();
            impl.window.reset();
            return false;
        }
        impl.backend   = backend;
        impl.valoReady = true;
        return true;
    };

    bool up = false;
    if (tryVulkan)          up = bringUp(valo::Backend::Vulkan);
    if (!up && trySoftware) up = bringUp(valo::Backend::Software);
    if (!up) {
        std::fprintf(stderr, "[UILO] could not open a window on any VALO backend\n");
        return false;
    }

    impl.applyLayerOrder();
    if (!impl.createResources()) {
        impl.shutdownResources();
        return false;
    }

    std::fprintf(stderr, "[UILO] renderer: VALO (%s)\n",
                 impl.backend == valo::Backend::Vulkan ? "Vulkan" : "software");

    const Vec2u sz = getSize();
    m_lastWidth   = sz.x;
    m_lastHeight  = sz.y;
    m_msaa        = msaa;
    m_initialised = true;
    impl.stats = {};
    return true;
}

/*
    shutdown():
    - Params:   none
    - Returns:  none
    - Desc:     Releases everything init() created: GPU resources, VALO and the
                window.
*/
void Renderer::shutdown() {
    if (!m_initialised) return;
    if (m_impl) m_impl->shutdownResources();
    m_initialised = false;
}

/*
    getSize():
    - Params:   none
    - Returns:  Vec2u
    - Desc:     The window's size in pixels -- the space every element's bounds
                are expressed in.
*/
Vec2u Renderer::getSize() const {
    if (!m_impl->window) return {0u, 0u};
    const valo::Vec2u sz = m_impl->window->getSize();
    return { sz.x, sz.y };
}

/*
    window():
    - Params:   none
    - Returns:  valo::Window*
    - Desc:     The window UILO reads input from, null before init().
*/
valo::Window* Renderer::window() const { return m_impl->window.get(); }

/*
    setTitle(const std::string& title):
    - Params:   const std::string& title
    - Returns:  none
    - Desc:     Sets the window title.
*/
void Renderer::setTitle(const std::string& title) {
    if (m_impl->window) m_impl->window->setTitle(title);
}

/*
    getStats():
    - Params:   none
    - Returns:  RendererStats
    - Desc:     Draw-command and vertex counters for the previous frame, and how
                long VALO took to run it, for a HUD or profiling.
*/
RendererStats Renderer::getStats() const {
    return m_impl->stats;
}

/*
    setVsync(bool enabled):
    - Params:   bool enabled
    - Returns:  none
    - Desc:     Turns vertical sync on or off.
*/
void Renderer::setVsync(bool enabled) {
    if (m_impl->vsync == enabled) return;
    m_impl->vsync = enabled;
    if (m_initialised) m_impl->valo.setVsync(enabled);
}

/*
    getVsync():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether vertical sync is on.
*/
bool Renderer::getVsync() const {
    return m_impl->vsync;
}

/*
    setFramerateLimit(float fps):
    - Params:   float fps
    - Returns:  none
    - Desc:     Caps the frame rate by sleeping out the remainder of each frame.
                0 removes the cap. Useful with vsync off, where an unbounded
                loop would spin the GPU for no visible benefit.
*/
void Renderer::setFramerateLimit(float fps) {
    if (fps <= 0.f || !std::isfinite(fps)) {
        m_frameInterval = 0.0;
        m_nextFrameTick = 0;
        return;
    }
    m_frameInterval = 1.0 / (double)fps;
    m_nextFrameTick = 0;   /* re-anchor on next endFrame() */
}

/*
    getFramerateLimit():
    - Params:   none
    - Returns:  float
    - Desc:     The current frame-rate cap, 0 when uncapped.
*/
float Renderer::getFramerateLimit() const {
    return m_frameInterval > 0.0 ? (float)(1.0 / m_frameInterval) : 0.f;
}

/*
    setCursor(CursorType type):
    - Params:   CursorType type
    - Returns:  none
    - Desc:     Applies a cursor shape to the window. VALO caches the system
                cursors.
*/
void Renderer::setCursor(CursorType type) {
    if (!m_impl->window) return;
    valo::CursorShape shape = valo::CursorShape::Arrow;
    switch (type) {
        case CursorType::Arrow:          shape = valo::CursorShape::Arrow;     break;
        case CursorType::Hand:           shape = valo::CursorShape::Hand;      break;
        case CursorType::SizeHorizontal: shape = valo::CursorShape::ResizeEW;  break;
        case CursorType::SizeVertical:   shape = valo::CursorShape::ResizeNS;  break;
        case CursorType::Text:           shape = valo::CursorShape::Text;      break;
        case CursorType::Crosshair:      shape = valo::CursorShape::Crosshair; break;
    }
    m_impl->window->setCursor(shape);
}

/*
    beginFrame():
    - Params:   none
    - Returns:  none
    - Desc:     Starts a frame: picks up a window resize, and resets the arena,
                the batch, the clip and target stacks and any rotation left set
                by the previous frame. Nothing reaches VALO until endFrame.
*/
void Renderer::beginFrame() {
    if (!m_initialised) return;
    auto& impl = *m_impl;

    const Vec2u sz = getSize();
    if (sz.x != m_lastWidth || sz.y != m_lastHeight) {
        if (sz.x > 0 && sz.y > 0) impl.valo.resize();
        m_lastWidth  = sz.x;
        m_lastHeight = sz.y;
    }
    impl.frameSize = sz;

    impl.verts.clear();
    impl.idx.clear();
    impl.batch = Impl::Batch{};
    impl.submittedCommands = 0;
    impl.clearColor = Color{0, 0, 0, 255};

    /* Defensively clear any rotation left set by user code from the last frame. */
    clearRotation();
    /* Defensive reset: clip and target stacks should be balanced each frame. */
    impl.scissorTop = 0;
    impl.roundClipTop = 0;
    impl.scissorOverflowDepth = 0;
    impl.roundClipOverflowDepth = 0;
    impl.targetTop = 0;
    ++impl.clipVersion;
}

/*
    endFrame():
    - Params:   none
    - Returns:  none
    - Desc:     Submits the last batch, uploads the arena and any glyphs packed
                this frame, and hands the frame to VALO: the clear colour the
                frame ended up with is applied first, then every command in
                submission order. A minimised window renders nothing.
*/
void Renderer::endFrame() {
    if (!m_initialised) return;
    auto& impl = *m_impl;
    impl.flushBatch();

    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();

    if (impl.frameSize.x == 0 || impl.frameSize.y == 0) {
        /* Minimised: beginShape queued nothing, and there is nothing to present
           into. Keep the loop from spinning until the window comes back. */
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return;
    }

    impl.uploadDirtyAtlases();
    if (!impl.verts.empty()) {
        impl.valo.updateVertexBuffer(impl.vb, impl.verts.data(), impl.verts.size());
        impl.valo.updateIndexBuffer(impl.ib, impl.idx.data(), impl.idx.size());
    }

    impl.valo.setLayerClearColor(valo::VALO::BASE_LAYER, toValoColor(impl.clearColor));
    impl.valo.beginFrame();
    impl.valo.endFrame();

    auto& stats = impl.stats;
    stats.numDraw     = impl.submittedCommands;
    stats.numVertices = static_cast<uint32_t>(impl.verts.size());
    stats.cpuTimeMs   = std::chrono::duration<double, std::milli>(clock::now() - t0).count();
    stats.gpuTimeMs   = 0.0;

    sleepUntilNextFrame(m_frameInterval, m_nextFrameTick);
}

/*
    createFrameBuffer(Vec2u size):
    - Params:   Vec2u size
    - Returns:  FrameBuffer
    - Desc:     A render target with its own layer. Its layer runs before the
                window's, and it clears to transparent at the start of every
                frame.
*/
FrameBuffer Renderer::createFrameBuffer(Vec2u size) {
    FrameBuffer fb;
    if (!m_initialised || size.x == 0 || size.y == 0) return fb;
    auto& impl = *m_impl;

    const valo::RenderTargetID target = impl.valo.createTarget(size.x, size.y);
    const valo::LayerID layer = impl.valo.addLayer(target, valo::Color{0.f, 0.f, 0.f, 0.f});
    impl.fbLayers.push_back(layer);
    impl.applyLayerOrder();

    assert(target < UINT16_MAX && layer < UINT16_MAX);
    fb.handle = static_cast<uint16_t>(target);
    fb.viewId = static_cast<uint16_t>(layer);
    fb.size   = size;
    return fb;
}

/*
    resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize):
    - Params:   FrameBuffer& fb, Vec2u newSize
    - Returns:  none
    - Desc:     Resizes a framebuffer by recreating it.
*/
void Renderer::resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize) {
    if (fb.size == newSize) return;
    destroyFrameBuffer(fb);
    fb = createFrameBuffer(newSize);
}

/*
    destroyFrameBuffer(FrameBuffer& fb):
    - Params:   FrameBuffer& fb
    - Returns:  none
    - Desc:     Destroys a framebuffer's target and retires its layer.
*/
void Renderer::destroyFrameBuffer(FrameBuffer& fb) {
    if (!fb.valid() || !m_initialised) return;
    auto& impl = *m_impl;
    impl.flushBatch();
    impl.valo.destroyTarget(fb.handle);
    impl.valo.removeLayer(fb.viewId);
    impl.fbLayers.erase(std::remove(impl.fbLayers.begin(), impl.fbLayers.end(),
                                    valo::LayerID(fb.viewId)),
                        impl.fbLayers.end());
    impl.applyLayerOrder();
    fb.handle = UINT16_MAX;
    fb.viewId = UINT16_MAX;
}

/*
    pushFrameBuffer(FrameBuffer& fb):
    - Params:   FrameBuffer& fb
    - Returns:  none
    - Desc:     Redirects drawing into a framebuffer until the matching pop.
                Used to render a subtree offscreen so it can be composited as a
                unit.
*/
void Renderer::pushFrameBuffer(FrameBuffer& fb) {
    auto& impl = *m_impl;
    if (!fb.valid()) return;
    assert(impl.targetTop < Impl::kMaxTargetStack);
    if (impl.targetTop >= Impl::kMaxTargetStack) return;
    impl.targetStack[impl.targetTop++] = { valo::LayerID(fb.viewId), fb.size };
}

/*
    popFrameBuffer():
    - Params:   none
    - Returns:  none
    - Desc:     Returns drawing to whatever target was active before the
                matching push.
*/
void Renderer::popFrameBuffer() {
    if (m_impl->targetTop > 0) --m_impl->targetTop;
}

/*
    drawFrameBuffer(const FrameBuffer& fb, Vec2f dest, Vec2f size, Color tint):
    - Params:   const FrameBuffer& fb, Vec2f dest, Vec2f size, Color tint
    - Returns:  none
    - Desc:     Draws a framebuffer's contents as a tinted quad, which is how an
                offscreen subtree is composited back into the scene.
*/
void Renderer::drawFrameBuffer(const FrameBuffer& fb, Vec2f dest, Vec2f size,
                                Color tint) {
    if (!fb.valid()) return;
    Texture tex;
    tex.handle = fb.handle;
    tex.width  = static_cast<uint16_t>(fb.size.x);
    tex.height = static_cast<uint16_t>(fb.size.y);
    drawImage(Rectf{dest, size}, tex, tint);
}

/*
    pushScissor(Rectf b):
    - Params:   Rectf b
    - Returns:  none
    - Desc:     Clips subsequent drawing to a rectangle, intersected with the
                clip already in force. Draws pick the top of the stack up as
                their command's scissor.
*/
void Renderer::pushScissor(Rectf b) {
    auto& impl = *m_impl;
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

    /* Quantize to integer pixel bounds conservatively: floor min edges and
       ceil max edges so clipping remains symmetric and stable as UI scale. */
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

    impl.scissorStack[impl.scissorTop++] = { x0, y0, w, h };
}

/*
    popScissor():
    - Params:   none
    - Returns:  none
    - Desc:     Restores the clip rectangle in force before the matching push.
*/
void Renderer::popScissor() {
    if (m_impl->scissorOverflowDepth > 0) {
        --m_impl->scissorOverflowDepth;
        return;
    }
    if (m_impl->scissorTop > 0) --m_impl->scissorTop;
}

/*
    pushRoundClip(Rectf b, float radius):
    - Params:   Rectf b, float radius
    - Returns:  none
    - Desc:     Clips to a rounded rectangle. A radius of 0 is a plain scissor;
                anything larger also feeds the rounded mask the shaders test
                against, so content cannot spill past a corner.
*/
void Renderer::pushRoundClip(Rectf b, float radius) {
    auto& impl = *m_impl;
    pushScissor(b);
    if (impl.roundClipTop >= Impl::kMaxRoundClip) {
        ++impl.roundClipOverflowDepth;
        return;
    }
    ++impl.clipVersion;   /* stack changes below; invalidate the clip cache */
    float r = std::max(0.f, radius);
    if (r <= 0.f) {
        /* Plain rectangle: still record an "off" entry so pop balances and
           (when there's already a rounded parent clip) the parent clip still. */
        if (impl.roundClipTop == 0) {
            impl.roundClipStack[impl.roundClipTop++] = {0.f, 0.f, 0.f, 0.f, -1.f};
        } else {
            /* Inherit parent's clip verbatim so SDF mask doesn't change. */
            impl.roundClipStack[impl.roundClipTop] =
                impl.roundClipStack[impl.roundClipTop - 1];
            impl.roundClipTop++;
        }
        return;
    }
    /* Snap rounded clip bounds to pixel edges to keep AA and corner shape
       visually consistent across scales. */
    const float x0 = std::floor(b.position.x);
    const float y0 = std::floor(b.position.y);
    const float x1 = std::ceil(b.position.x + b.size.x);
    const float y1 = std::ceil(b.position.y + b.size.y);

    float halfW = std::max(0.f, (x1 - x0) * 0.5f);
    float halfH = std::max(0.f, (y1 - y0) * 0.5f);
    float cx    = x0 + halfW;
    float cy    = y0 + halfH;
    /* Do NOT intersect the child's rounded-rect SDF with the parent's rect. */
    r = std::min(r, std::min(halfW, halfH));
    impl.roundClipStack[impl.roundClipTop++] = {cx, cy, halfW, halfH, r};
}

/*
    popRoundClip():
    - Params:   none
    - Returns:  none
    - Desc:     Restores the rounded clip in force before the matching push.
*/
void Renderer::popRoundClip() {
    auto& impl = *m_impl;
    if (impl.roundClipOverflowDepth > 0) {
        --impl.roundClipOverflowDepth;
    } else if (impl.roundClipTop > 0) {
        --impl.roundClipTop;
        ++impl.clipVersion;
    }
    popScissor();
}

/*
    setRotation(float degrees, Vec2f pivot):
    - Params:   float degrees, Vec2f pivot
    - Returns:  none
    - Desc:     Rotates subsequent drawing about a pivot. Applied CPU-side as
                vertices are emitted, so geometry already in the arena keeps the
                rotation it was emitted under.
*/
void Renderer::setRotation(float degrees, Vec2f pivot) {
    auto& r = m_impl->rotation;
    r.pivotX   = pivot.x;
    r.pivotY   = pivot.y;
    r.angleDeg = degrees;
    const float rad = degrees * (3.14159265f / 180.f);
    r.cosA = std::cos(rad);
    r.sinA = std::sin(rad);
    r.enabled = std::abs(r.sinA) > 1e-6f || std::abs(r.cosA - 1.f) > 1e-6f;
}

/*
    rotate(float deltaDegrees):
    - Params:   float deltaDegrees
    - Returns:  none
    - Desc:     Rotates subsequent drawing about the current origin,
                accumulating with any rotation already in force.
*/
void Renderer::rotate(float deltaDegrees) {
    auto& r = m_impl->rotation;
    r.angleDeg += deltaDegrees;
    const float rad = r.angleDeg * (3.14159265f / 180.f);
    r.cosA = std::cos(rad);
    r.sinA = std::sin(rad);
    r.enabled = std::abs(r.sinA) > 1e-6f || std::abs(r.cosA - 1.f) > 1e-6f;
}

/*
    clearRotation():
    - Params:   none
    - Returns:  none
    - Desc:     Drops any accumulated rotation.
*/
void Renderer::clearRotation() {
    auto& r = m_impl->rotation;
    r.angleDeg = 0.f;
    r.cosA = 1.f;
    r.sinA = 0.f;
    r.enabled = false;
}

/*
    clear(Color color):
    - Params:   Color color
    - Returns:  none
    - Desc:     Sets the colour the current target is cleared to at the start of
                the frame. The last call in a frame wins and is applied before
                any of the frame's draws, whenever it was made.
*/
void Renderer::clear(Color color) {
    auto& impl = *m_impl;
    if (impl.targetTop > 0) {
        if (m_initialised)
            impl.valo.setLayerClearColor(impl.currentLayer(), toValoColor(color));
        return;
    }
    impl.clearColor = color;
}

/* ============================================================================
   Shape drawing. */

static constexpr float kDeg2Rad = 3.14159265f / 180.f;

namespace {

/*
    avgColor(Color a, Color b, Color c, Color d):
    - Params:   Color a, Color b, Color c, Color d
    - Returns:  Color
    - Desc:     Center-vertex color for gradient quads: the average of the four
                corners.
*/
inline Color avgColor(Color a, Color b, Color c, Color d) {
    return Color{
        uint8_t((a.r + b.r + c.r + d.r) / 4),
        uint8_t((a.g + b.g + c.g + d.g) / 4),
        uint8_t((a.b + b.b + c.b + d.b) / 4),
        uint8_t((a.a + b.a + c.a + d.a) / 4)};
}

/* Same alpha arithmetic the original vertex packing did, so skirts fade in the
   same steps. */
inline Color withAlpha(Color c, uint8_t a) { return Color{c.r, c.g, c.b, a}; }

} // anon

/*
    draw(const Rect& r):
    - Params:   const Rect& r
    - Returns:  none
    - Desc:     Queues an axis-aligned rectangle, with an optional inside border
                and per-corner gradient, into the batch.
*/
void Renderer::draw(const Rect& r) {
    auto& impl = *m_impl;
    if (!m_initialised || !impl.beginShape(Impl::ModeSolid, valo::EMPTY_TEXTURE)) return;

    float x = r.position.x, y = r.position.y;
    float w = r.size.x,     h = r.size.y;

    float x0 = x,     y0 = y;
    float x1 = x + w, y1 = y;
    float x2 = x + w, y2 = y + h;
    float x3 = x,     y3 = y + h;
    impl.rotPt(x0, y0); impl.rotPt(x1, y1);
    impl.rotPt(x2, y2); impl.rotPt(x3, y3);

    /* Vertex order is TL, TR, BR, BL. */
    Color c0, c1, c2, c3;
    if (r.gradient) {
        c0 = r.colorTL; c1 = r.colorTR;
        c2 = r.colorBR; c3 = r.colorBL;
    } else {
        c0 = c1 = c2 = c3 = r.fillColor;
    }

    const uint32_t b = impl.pushVertex(x0, y0, c0);
    impl.pushVertex(x1, y1, c1);
    impl.pushVertex(x2, y2, c2);
    impl.pushVertex(x3, y3, c3);
    if (!r.gradient) {
        impl.pushIndex(b + 0); impl.pushIndex(b + 1); impl.pushIndex(b + 2);
        impl.pushIndex(b + 0); impl.pushIndex(b + 2); impl.pushIndex(b + 3);
    } else {
        /* Two triangles interpolate a 4-corner gradient with a visible seam
           along the shared diagonal whenever the corner colors aren't. */
        float cx = x + w * 0.5f, cy = y + h * 0.5f;
        impl.rotPt(cx, cy);
        impl.pushVertex(cx, cy, avgColor(r.colorTL, r.colorTR, r.colorBL, r.colorBR));
        static constexpr uint32_t kFan[12] = {0,1,4, 1,2,4, 2,3,4, 3,0,4};
        for (uint32_t i : kFan) impl.pushIndex(b + i);
    }

    if (r.outlineThickness > 0.f && r.outlineColor.a > 0) {
        float t = r.outlineThickness;
        Rect top    { r.position,        {w, t}, r.outlineColor };
        Rect bottom { {x, y + h - t},    {w, t}, r.outlineColor };
        Rect left   { r.position,        {t, h}, r.outlineColor };
        Rect right  { {x + w - t, y},    {t, h}, r.outlineColor };
        draw(top); draw(bottom); draw(left); draw(right);
    }
}

/*
    draw(const RoundedRect& rr):
    - Params:   const RoundedRect& rr
    - Returns:  none
    - Desc:     Queues a rounded rectangle. The fill is masked by a signed-
                distance test in the shader; the border is a true inside ring --
                four annular corners and four straight bars -- drawn after the
                fill so it is visible over a transparent one.
*/
void Renderer::draw(const RoundedRect& rr) {
    auto& impl = *m_impl;
    if (!m_initialised || impl.scissorEmpty()) return;

    float r = std::min(rr.radius, std::min(rr.size.x, rr.size.y) * 0.5f);
    if (r <= 0.f) {
        Rect rect{rr.position, rr.size, rr.fillColor,
                  rr.outlineColor, rr.outlineThickness};
        rect.gradient = rr.gradient;
        rect.colorTL = rr.colorTL; rect.colorTR = rr.colorTR;
        rect.colorBL = rr.colorBL; rect.colorBR = rr.colorBR;
        draw(rect);
        return;
    }

    auto submitQuad = [&](float x, float y, float w, float h,
                          Color cTL, Color cTR, Color cBR, Color cBL,
                          bool gradient) {
        if (!impl.beginShape(Impl::ModeSolid, valo::EMPTY_TEXTURE)) return;
        float x0 = x,     y0 = y;
        float x1 = x + w, y1 = y;
        float x2 = x + w, y2 = y + h;
        float x3 = x,     y3 = y + h;
        impl.rotPt(x0, y0); impl.rotPt(x1, y1);
        impl.rotPt(x2, y2); impl.rotPt(x3, y3);
        const uint32_t b = impl.pushVertex(x0, y0, cTL);
        impl.pushVertex(x1, y1, cTR);
        impl.pushVertex(x2, y2, cBR);
        impl.pushVertex(x3, y3, cBL);
        if (gradient) {
            /* Center-vertex fan: see draw(Rect) — avoids the diagonal seam on
               non-coplanar corner colors. */
            float cx = x + w * 0.5f, cy = y + h * 0.5f;
            impl.rotPt(cx, cy);
            impl.pushVertex(cx, cy, avgColor(cTL, cTR, cBL, cBR));
            static constexpr uint32_t idxFan[12] = {0,1,4, 1,2,4, 2,3,4, 3,0,4};
            for (uint32_t i : idxFan) impl.pushIndex(b + i);
        } else {
            static constexpr uint32_t idxQuad[6] = {0,1,2, 0,2,3};
            for (uint32_t i : idxQuad) impl.pushIndex(b + i);
        }
    };

    pushRoundClip({rr.position, rr.size}, r);
    if (rr.gradient) {
        submitQuad(rr.position.x, rr.position.y, rr.size.x, rr.size.y,
                   rr.colorTL, rr.colorTR, rr.colorBR, rr.colorBL, true);
    } else {
        submitQuad(rr.position.x, rr.position.y, rr.size.x, rr.size.y,
                   rr.fillColor, rr.fillColor, rr.fillColor, rr.fillColor, false);
    }
    popRoundClip();
    if (rr.outlineThickness > 0.f && rr.outlineColor.a > 0) {
        /* A true ring -- four annular corners plus four straight edges --
           drawn *inside* the bounds. */
        const float t   = std::min(rr.outlineThickness,
                                   std::min(rr.size.x, rr.size.y) * 0.5f);
        const float x   = rr.position.x, y = rr.position.y;
        const float w   = rr.size.x,     h = rr.size.y;
        const float ir  = std::max(0.f, r - t);
        const Color oc  = rr.outlineColor;
        /* Enough segments to stay smooth at the radii a UI uses. */
        const int   seg = std::max(4, static_cast<int>(r * 0.75f) + 4);

        /* drawArc antialiases by fading out over roughly a pixel past each
           radial edge, so a band handed to it straight reads about a pixel. */
        constexpr float kArcFade = 0.5f;
        const float aOuter = std::max(0.f, r  - kArcFade);
        const float aInner = std::min(aOuter, std::max(0.f, ir + kArcFade));

        /* Corners. Angles are cartesian with 0 = +x and y running down the
           screen, so an increasing sweep travels clockwise as drawn. */
        drawArc({x + r,     y + r    }, aInner, aOuter, 180.f, 270.f, oc, seg);   /* top-left */
        drawArc({x + w - r, y + r    }, aInner, aOuter, 270.f, 360.f, oc, seg);   /* top-right */
        drawArc({x + w - r, y + h - r}, aInner, aOuter,   0.f,  90.f, oc, seg);   /* bottom-right */
        drawArc({x + r,     y + h - r}, aInner, aOuter,  90.f, 180.f, oc, seg);   /* bottom-left */

        /* Edges, spanning the straight runs between the corners. */
        const float midW = std::max(0.f, w - 2.f * r);
        const float midH = std::max(0.f, h - 2.f * r);
        if (midW > 0.f) {
            draw(Rect{{x + r, y        }, {midW, t}, oc});
            draw(Rect{{x + r, y + h - t}, {midW, t}, oc});
        }
        if (midH > 0.f) {
            draw(Rect{{x,         y + r}, {t, midH}, oc});
            draw(Rect{{x + w - t, y + r}, {t, midH}, oc});
        }
    }
}

/*
    draw(const Circle& c):
    - Params:   const Circle& c
    - Returns:  none
    - Desc:     Queues a circle as a rounded rect with radius == half-size, so the
                fragment shader's SDF gives it sub-pixel antialiasing.
*/
void Renderer::draw(const Circle& c) {
    auto& impl = *m_impl;
    if (!m_initialised || impl.scissorEmpty()) return;
    if (c.radius <= 0.f || c.fillColor.a == 0) return;

    const float r   = c.radius;
    const float pad = 1.f;
    const float x   = c.center.x - r - pad;
    const float y   = c.center.y - r - pad;
    const float w   = (r + pad) * 2.f;
    const float h   = (r + pad) * 2.f;

    pushRoundClip({{c.center.x - r, c.center.y - r}, {r * 2.f, r * 2.f}}, r);

    if (impl.beginShape(Impl::ModeSolid, valo::EMPTY_TEXTURE)) {
        float x0 = x,     y0 = y;
        float x1 = x + w, y1 = y;
        float x2 = x + w, y2 = y + h;
        float x3 = x,     y3 = y + h;
        impl.rotPt(x0, y0); impl.rotPt(x1, y1);
        impl.rotPt(x2, y2); impl.rotPt(x3, y3);
        const uint32_t b = impl.pushVertex(x0, y0, c.fillColor);
        impl.pushVertex(x1, y1, c.fillColor);
        impl.pushVertex(x2, y2, c.fillColor);
        impl.pushVertex(x3, y3, c.fillColor);
        static constexpr uint32_t idxQuad[6] = {0,1,2, 0,2,3};
        for (uint32_t i : idxQuad) impl.pushIndex(b + i);
    }

    popRoundClip();
}

/*
    draw(const Triangle& t):
    - Params:   const Triangle& t
    - Returns:  none
    - Desc:     Queues a triangle.
*/
void Renderer::draw(const Triangle& t) {
    auto& impl = *m_impl;
    if (!m_initialised || !impl.beginShape(Impl::ModeSolid, valo::EMPTY_TEXTURE)) return;

    float ax = t.a.x, ay = t.a.y;
    float bx = t.b.x, by = t.b.y;
    float cx = t.c.x, cy = t.c.y;
    impl.rotPt(ax, ay);
    impl.rotPt(bx, by);
    impl.rotPt(cx, cy);
    const uint32_t b = impl.pushVertex(ax, ay, t.fillColor);
    impl.pushVertex(bx, by, t.fillColor);
    impl.pushVertex(cx, cy, t.fillColor);
    impl.pushIndex(b + 0); impl.pushIndex(b + 1); impl.pushIndex(b + 2);
}

/*
    draw(const Line& l):
    - Params:   const Line& l
    - Returns:  none
    - Desc:     Queues a single line as a quad expanded to its thickness, with a
                one-pixel alpha skirt either side for antialiasing.
*/
void Renderer::draw(const Line& l) {
    auto& impl = *m_impl;
    if (!m_initialised || impl.scissorEmpty()) return;

    float dx = l.end.x - l.start.x;
    float dy = l.end.y - l.start.y;
    float len = std::sqrt(dx*dx + dy*dy);
    if (len < 0.001f) return;
    if (!impl.beginShape(Impl::ModeSolid, valo::EMPTY_TEXTURE)) return;

    /* Unit normal (perp to line direction). */
    const float ux = -dy / len;
    const float uy =  dx / len;
    const float half = l.thickness * 0.5f;
    const float pad  = 1.f;   /* skirt width in pixels for edge AA */

    /* Four rows along the normal: outer skirt, edge, edge, outer skirt. */
    const float offs[4]   = {  half + pad,  half, -half, -half - pad };
    const uint8_t alphas[4] = { 0, 255, 255, 0 };

    const uint8_t baseA = l.color.a;
    uint32_t base = 0;
    for (int ep = 0; ep < 2; ++ep) {
        const float bx = (ep == 0) ? l.start.x : l.end.x;
        const float by = (ep == 0) ? l.start.y : l.end.y;
        for (int r = 0; r < 4; ++r) {
            float x = bx + ux * offs[r];
            float y = by + uy * offs[r];
            impl.rotPt(x, y);
            uint8_t a = (uint8_t)((uint16_t)baseA * (uint16_t)alphas[r] / 255);
            const uint32_t v = impl.pushVertex(x, y, withAlpha(l.color, a));
            if (ep == 0 && r == 0) base = v;
        }
    }

    /* 3 quads (skirt, body, skirt) between the two endpoints. */
    for (int r = 0; r < 3; ++r) {
        const uint32_t v00 = base + (uint32_t)(0*4 + r);
        const uint32_t v01 = base + (uint32_t)(0*4 + r + 1);
        const uint32_t v10 = base + (uint32_t)(1*4 + r);
        const uint32_t v11 = base + (uint32_t)(1*4 + r + 1);
        impl.pushIndex(v00); impl.pushIndex(v01); impl.pushIndex(v11);
        impl.pushIndex(v00); impl.pushIndex(v11); impl.pushIndex(v10);
    }
}

/*
    drawLines(const Line* lines, size_t count):
    - Params:   const Line* lines, size_t count
    - Returns:  none
    - Desc:     Draws many lines in a single batch, which is what keeps a
                subdivision grid or a waveform to one draw call instead of
                hundreds.
*/
void Renderer::drawLines(const Line* lines, size_t count) {
    auto& impl = *m_impl;
    if (!m_initialised || !lines || count == 0) return;
    if (!impl.beginShape(Impl::ModeSolid, valo::EMPTY_TEXTURE)) return;

    for (size_t i = 0; i < count; ++i) {
        const Line& l = lines[i];
        float dx = l.end.x - l.start.x;
        float dy = l.end.y - l.start.y;
        float len2 = dx*dx + dy*dy;
        if (len2 < 1e-6f) continue;
        float len = std::sqrt(len2);
        float nx = -dy / len * l.thickness * 0.5f;
        float ny =  dx / len * l.thickness * 0.5f;
        float x0 = l.start.x + nx, y0 = l.start.y + ny;
        float x1 = l.start.x - nx, y1 = l.start.y - ny;
        float x2 = l.end.x   - nx, y2 = l.end.y   - ny;
        float x3 = l.end.x   + nx, y3 = l.end.y   + ny;
        impl.rotPt(x0, y0); impl.rotPt(x1, y1);
        impl.rotPt(x2, y2); impl.rotPt(x3, y3);
        const uint32_t b = impl.pushVertex(x0, y0, l.color);
        impl.pushVertex(x1, y1, l.color);
        impl.pushVertex(x2, y2, l.color);
        impl.pushVertex(x3, y3, l.color);
        impl.pushIndex(b + 0); impl.pushIndex(b + 1); impl.pushIndex(b + 2);
        impl.pushIndex(b + 0); impl.pushIndex(b + 2); impl.pushIndex(b + 3);
    }
}

/*
    drawArc(...):
    - Params:   Vec2f center, float innerR, float outerR, float startDeg, float
                endDeg, Color color, int segments
    - Returns:  none
    - Desc:     Draws an annular sector between two radii, antialiased by fading
                out over about a pixel past each edge, angular and radial alike.
                A zero inner radius makes it a filled wedge instead, in which
                case the inner fade is dropped.
*/
void Renderer::drawArc(Vec2f center, float innerR, float outerR,
                       float startDeg, float endDeg, Color color, int segments) {
    auto& impl = *m_impl;
    if (!m_initialised || impl.scissorEmpty()) return;
    if (color.a == 0) return;

    if (innerR > outerR) std::swap(innerR, outerR);
    if (outerR <= 0.f) return;
    if (innerR < 0.f) innerR = 0.f;
    if (!impl.beginShape(Impl::ModeSolid, valo::EMPTY_TEXTURE)) return;

    int segs = std::max(1, segments);
    /* We emit (segs + 2 angular skirt) slices x 4 radial rows. */
    constexpr int kMaxSlices = (65535 / 6);
    if (segs + 2 > kMaxSlices) segs = kMaxSlices - 2;

    const float startRad = startDeg * kDeg2Rad;
    const float endRad   = endDeg   * kDeg2Rad;
    const float step     = (endRad - startRad) / (float)segs;

    /* Angular skirt width = ~1 pixel of arc length at outerR. */
    const float angSkirt = (outerR > 0.5f) ? (1.f / outerR) : 0.f;
    const float startSk  = startRad - (step >= 0.f ? angSkirt : -angSkirt);
    const float endSk    = endRad   + (step >= 0.f ? angSkirt : -angSkirt);

    /* Radial rows: outer skirt | outer | inner | inner skirt. */
    const float innerSk = std::max(0.f, innerR - 1.f);
    const float outerSk = outerR + 1.f;
    const float radii[4] = { outerSk, outerR, innerR, innerSk };
    /* Alpha modulation per row (skirts fade to 0). */
    const uint8_t alphaMul[4] = { 0, 255, 255, 0 };
    /* If innerR <= 0 we draw a wedge; pull the inner skirt in and keep it at
       alpha 1 (no inner hole, no AA needed). */
    const bool solidCore = innerR <= 0.f;

    const int sliceCount = segs + 3;   /* [-1, 0..segs, segs+1] */
    /* Quads per slice gap: 3 normally, or 2 if solidCore (skip inner-skirt
       row). */
    const int rowsPerGap = solidCore ? 2 : 3;

    auto sliceAngle = [&](int s) -> float {
        if (s == 0)              return startSk;
        if (s == sliceCount - 1) return endSk;
        return startRad + step * (float)(s - 1);
    };
    auto sliceAngularAlpha = [&](int s) -> uint8_t {
        return (s == 0 || s == sliceCount - 1) ? 0 : 255;
    };

    uint32_t base = 0;
    for (int s = 0; s < sliceCount; ++s) {
        const float a   = sliceAngle(s);
        const float ca  = std::cos(a);
        const float sa  = std::sin(a);
        const uint8_t am = sliceAngularAlpha(s);
        for (int r = 0; r < 4; ++r) {
            float px = center.x + radii[r] * ca;
            float py = center.y + radii[r] * sa;
            impl.rotPt(px, py);
            /* Combine radial-skirt alpha with angular-skirt alpha
               (multiplicative). Solid-core forces inner skirt alpha to 1. */
            uint8_t ra = (solidCore && r == 3) ? 255 : alphaMul[r];
            uint16_t a16 = (uint16_t)ra * (uint16_t)am;
            uint8_t finalA = (uint8_t)((a16 + 127) / 255);
            const Color cc{color.r, color.g, color.b,
                           (uint8_t)((uint16_t)color.a * (uint16_t)finalA / 255)};
            const uint32_t v = impl.pushVertex(px, py, cc);
            if (s == 0 && r == 0) base = v;
        }
    }

    for (int s = 0; s < sliceCount - 1; ++s) {
        for (int r = 0; r < rowsPerGap; ++r) {
            const uint32_t v00 = base + (uint32_t)(s*4 + r);
            const uint32_t v01 = base + (uint32_t)(s*4 + r + 1);
            const uint32_t v10 = base + (uint32_t)((s+1)*4 + r);
            const uint32_t v11 = base + (uint32_t)((s+1)*4 + r + 1);
            impl.pushIndex(v00); impl.pushIndex(v01); impl.pushIndex(v11);
            impl.pushIndex(v00); impl.pushIndex(v11); impl.pushIndex(v10);
        }
    }
}

} // namespace uilo
