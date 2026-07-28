// Headless backend for UILO's UILO_WT build.
//
// The Wt bridge builds a UILO element tree and reads it; it never ticks or
// draws it. That means every GPU and OS-input entry point can collapse to a
// no-op, which is what this file provides:
//
//   * `Renderer` -- the whole class, so bgfx and SDL are neither compiled
//     nor linked. Renderer.hpp itself only forward-declares those types, so
//     the header stays usable as-is.
//   * The SDL functions declared by wt/shim/SDL3/SDL.h.
//   * The macOS trackpad / live-resize shims that UILO.cpp calls. (UILO's own
//     MacStubs.cpp is guarded on !__APPLE__, so a headless macOS build would
//     otherwise be missing them.)
//
// Nothing here is on a code path the web app reaches. If a headless program
// ever does call update()/render(), it draws nothing rather than crashing.

#include "../renderer/Renderer.hpp"
#include "../platform/MacScroll.hpp"
#include "../platform/MacWindow.hpp"

#include <SDL3/SDL.h>

// ---------------------------------------------------------------------------
// SDL
// ---------------------------------------------------------------------------

SDL_MouseButtonFlags SDL_GetMouseState(float* x, float* y) {
    if (x) *x = 0.f;
    if (y) *y = 0.f;
    return 0;
}

Uint64      SDL_GetTicks()                  { return 0; }
SDL_Keymod  SDL_GetModState()               { return SDL_KMOD_NONE; }

const bool* SDL_GetKeyboardState(int* numkeys) {
    // A full always-up scancode table. UILO scans [0, numkeys) looking for any
    // key held down, and uilo::Keybinds indexes this directly by scancode, so
    // the table has to span the whole range rather than a single entry.
    static constexpr int kScancodeCount = 512; // SDL_SCANCODE_COUNT
    static const bool kNoKeysDown[kScancodeCount] = { false };
    if (numkeys) *numkeys = kScancodeCount;
    return kNoKeysDown;
}

char* SDL_GetClipboardText()            { return nullptr; }
bool  SDL_SetClipboardText(const char*) { return true; }
void  SDL_free(void*)                   {}

bool SDL_StartTextInput(SDL_Window*)  { return true; }
bool SDL_StopTextInput(SDL_Window*)   { return true; }
bool SDL_TextInputActive(SDL_Window*) { return false; }

bool SDL_AddEventWatch(SDL_EventFilter, void*) { return true; }

// The browser is the input device on this backend, so the queue is always empty
// and there is no cursor to capture or warp.
bool SDL_PollEvent(SDL_Event*) { return false; }

bool SDL_SetWindowRelativeMouseMode(SDL_Window*, bool) { return true; }
bool SDL_GetWindowRelativeMouseMode(SDL_Window*)       { return false; }

void SDL_GetRelativeMouseState(float* x, float* y) {
    if (x) *x = 0.f;
    if (y) *y = 0.f;
}

float SDL_GetWindowDisplayScale(SDL_Window*) { return 1.f; }

const char* SDL_GetError() { return ""; }

bool SDL_GetWindowSize(SDL_Window*, int* w, int* h) {
    if (w) *w = 0;
    if (h) *h = 0;
    return true;
}

bool SDL_GetWindowSizeInPixels(SDL_Window*, int* w, int* h) {
    if (w) *w = 0;
    if (h) *h = 0;
    return true;
}

SDL_PropertiesID SDL_GetWindowProperties(SDL_Window*) { return 0; }

void* SDL_GetPointerProperty(SDL_PropertiesID, const char*, void* default_value) {
    return default_value;
}

namespace uilo {

// ---------------------------------------------------------------------------
// macOS shims
// ---------------------------------------------------------------------------

bool configureMacWindowForLiveResize(void*) { return false; }
bool installMacScrollMonitor(std::function<bool(float, float, bool)>) { return false; }
bool installMacZoomMonitor(std::function<bool(float)>) { return false; }
void tickMacScrollMomentum(float) {}
void cancelMacScrollMomentum() {}

// ---------------------------------------------------------------------------
// Renderer
// ---------------------------------------------------------------------------

// Renderer holds a unique_ptr<Impl>, so Impl has to be complete where the
// destructor is emitted. Empty is enough -- there is no GPU state to keep.
struct Renderer::Impl {};

Renderer::Renderer()  = default;
Renderer::~Renderer() = default;

bool Renderer::init(uint32_t width, uint32_t height, const std::string&, uint8_t msaa) {
    m_lastWidth   = width;
    m_lastHeight  = height;
    m_msaa        = msaa;
    m_initialised = true;
    return true;
}

bool Renderer::attach(SDL_Window*, uint16_t) {
    m_ownsContext = false;
    m_initialised = true;
    return true;
}

void Renderer::shutdown()   { m_initialised = false; }
void Renderer::beginFrame() {}
void Renderer::endFrame()   {}

Vec2u Renderer::getSize() const { return { m_lastWidth, m_lastHeight }; }

void  Renderer::setTitle(const std::string&) {}
void  Renderer::setVsync(bool) {}
bool  Renderer::getVsync() const { return false; }
void  Renderer::setFramerateLimit(float) {}
float Renderer::getFramerateLimit() const { return 0.f; }

RendererStats Renderer::getStats() const { return {}; }

void Renderer::setCursor(CursorType) {}

void Renderer::draw(const Rect&)        {}
void Renderer::draw(const RoundedRect&) {}
void Renderer::draw(const Circle&)      {}
void Renderer::draw(const Triangle&)    {}
void Renderer::draw(const Line&)        {}

void Renderer::drawLines(const Line*, size_t) {}
void Renderer::drawArc(Vec2f, float, float, float, float, Color, int) {}

Texture Renderer::loadTexture(const std::string&) { return {}; }
void    Renderer::destroyTexture(Texture& tex)    { tex = Texture{}; }

bool Renderer::loadImagePixels(const std::string&, std::vector<uint8_t>&,
                               uint32_t&, uint32_t&) {
    return false;
}

Texture Renderer::createTexture(uint16_t, uint16_t) { return {}; }
void    Renderer::updateTexture(const Texture&, const uint8_t*) {}

void Renderer::drawImage(const Rectf&, const Texture&, Color, Rectf, bool, bool, bool) {}

void Renderer::drawGlass(const Rectf&, const Material&, Color) {}
void Renderer::setMouseState(Vec2f pos) { m_mousePos = pos; }

Font Renderer::loadFont(const std::string&) { return {}; }

void Renderer::drawText(const std::string&, Vec2f, const Font&, float, Color) {}

TextMetrics Renderer::measureText(const std::string&, const Font&, float) { return {}; }

std::vector<Vec2f> Renderer::charPositions(const std::string&, const Font&, float) {
    return { Vec2f{0.f, 0.f} };
}

FrameBuffer Renderer::createFrameBuffer(Vec2u size) {
    FrameBuffer fb;
    fb.size = size;
    return fb;
}

void Renderer::resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize) { fb.size = newSize; }
void Renderer::destroyFrameBuffer(FrameBuffer& fb)               { fb = FrameBuffer{}; }
void Renderer::pushFrameBuffer(FrameBuffer&)                     {}
void Renderer::popFrameBuffer()                                  {}
void Renderer::drawFrameBuffer(const FrameBuffer&, Vec2f, Vec2f, Color) {}

void Renderer::clear(Color) {}

void Renderer::pushScissor(Rectf) {}
void Renderer::popScissor()       {}

void Renderer::pushRoundClip(Rectf, float) {}
void Renderer::popRoundClip()              {}

void Renderer::beginGlassSubtree() {}
void Renderer::endGlassSubtree()   {}

void Renderer::setRotation(float, Vec2f) {}
void Renderer::rotate(float)             {}
void Renderer::clearRotation()           {}

uint16_t Renderer::currentViewId() const { return 0; }
void     Renderer::submitOrtho(uint16_t, Vec2u) {}

} // namespace uilo
