/*
    HeadlessBackend.cpp:
    - Desc: The whole GPU and OS-input surface, stubbed out, for the UILO_WT
            build. The Wt bridge builds a UILO element tree and reads it but
            never ticks or draws it, so every one of these entry points can
            collapse to a no-op and bgfx and SDL are then neither compiled nor
            linked. Renderer.hpp only forward-declares those types, so the header
            stays usable unchanged.
    - Four groups live here: the entire Renderer class, the SDL functions
      declared by wt/shim/SDL3/SDL.h, the macOS trackpad and live-resize shims
      UILO.cpp calls -- UILO's own MacStubs.cpp is guarded on !__APPLE__, so a
      headless macOS build would otherwise be missing them -- and Pty, which
      Terminal links against but which cannot exist on the web at all.
    - Nothing here is on a path the web app reaches. A headless program that does
      call update() or render() draws nothing rather than crashing.
*/

#include "../renderer/Renderer.hpp"
#include "../platform/MacScroll.hpp"
#include "../platform/MacWindow.hpp"
#include "../platform/Pty.hpp"

#include <SDL3/SDL.h>

/*
    SDL_GetMouseState(float* x, float* y):
    - Params:   float* x, float* y
    - Returns:  SDL_MouseButtonFlags
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
SDL_MouseButtonFlags SDL_GetMouseState(float* x, float* y) {
    if (x) *x = 0.f;
    if (y) *y = 0.f;
    return 0;
}

/*
    SDL_GetTicks():
    - Params:   none
    - Returns:  Uint64
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
Uint64      SDL_GetTicks()                  { return 0; }
/*
    SDL_GetModState():
    - Params:   none
    - Returns:  SDL_Keymod
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
SDL_Keymod  SDL_GetModState()               { return SDL_KMOD_NONE; }

/*
    SDL_GetKeyboardState(int* numkeys):
    - Params:   int* numkeys
    - Returns:  const bool*
    - Desc:     A full always-up scancode table. UILO scans [0, numkeys) looking
                for any key held down, and uilo::Keybinds indexes this directly
                by scancode, so the table has to span the whole range rather
                than a single entry.
*/
const bool* SDL_GetKeyboardState(int* numkeys) {
    static constexpr int kScancodeCount = 512;   /* SDL_SCANCODE_COUNT */
    static const bool kNoKeysDown[kScancodeCount] = { false };
    if (numkeys) *numkeys = kScancodeCount;
    return kNoKeysDown;
}

/*
    SDL_GetClipboardText():
    - Params:   none
    - Returns:  char*
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
char* SDL_GetClipboardText()            { return nullptr; }
/*
    SDL_SetClipboardText(const char*):
    - Params:   const char*
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool  SDL_SetClipboardText(const char*) { return true; }
/*
    SDL_free(void*):
    - Params:   void*
    - Returns:  none
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
void  SDL_free(void*)                   {}

/*
    SDL_StartTextInput(SDL_Window*):
    - Params:   SDL_Window*
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool SDL_StartTextInput(SDL_Window*)  { return true; }
/*
    SDL_StopTextInput(SDL_Window*):
    - Params:   SDL_Window*
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool SDL_StopTextInput(SDL_Window*)   { return true; }
/*
    SDL_TextInputActive(SDL_Window*):
    - Params:   SDL_Window*
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool SDL_TextInputActive(SDL_Window*) { return false; }

/*
    SDL_AddEventWatch(SDL_EventFilter, void*):
    - Params:   SDL_EventFilter, void*
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool SDL_AddEventWatch(SDL_EventFilter, void*) { return true; }

/*
    SDL_PollEvent(SDL_Event*):
    - Params:   SDL_Event*
    - Returns:  bool
    - Desc:     The browser is the input device on this backend, so the queue is
                always empty and there is no cursor to capture or warp.
*/
bool SDL_PollEvent(SDL_Event*) { return false; }

/*
    SDL_SetWindowRelativeMouseMode(SDL_Window*, bool):
    - Params:   SDL_Window*, bool
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool SDL_SetWindowRelativeMouseMode(SDL_Window*, bool) { return true; }
/*
    SDL_GetWindowRelativeMouseMode(SDL_Window*):
    - Params:   SDL_Window*
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool SDL_GetWindowRelativeMouseMode(SDL_Window*)       { return false; }

/*
    SDL_GetRelativeMouseState(float* x, float* y):
    - Params:   float* x, float* y
    - Returns:  none
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
void SDL_GetRelativeMouseState(float* x, float* y) {
    if (x) *x = 0.f;
    if (y) *y = 0.f;
}

/*
    SDL_GetWindowDisplayScale(SDL_Window*):
    - Params:   SDL_Window*
    - Returns:  float
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
float SDL_GetWindowDisplayScale(SDL_Window*) { return 1.f; }

/*
    SDL_GetError():
    - Params:   none
    - Returns:  const char*
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
const char* SDL_GetError() { return ""; }

/*
    SDL_WasInit(SDL_InitFlags):
    - Params:   SDL_InitFlags
    - Returns:  SDL_InitFlags
    - Desc:     uilo::OS queries. There is no local display on this backend, so
                every one reports "not initialised / unknown" and OS returns its
                documented fallbacks (scale 1.0, zero sizes) rather than
                inventing values.
*/
SDL_InitFlags SDL_WasInit(SDL_InitFlags)                  { return 0; }
/*
    SDL_GetPrimaryDisplay():
    - Params:   none
    - Returns:  SDL_DisplayID
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
SDL_DisplayID SDL_GetPrimaryDisplay()                     { return 0; }
/*
    SDL_GetDisplayContentScale(SDL_DisplayID):
    - Params:   SDL_DisplayID
    - Returns:  float
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
float         SDL_GetDisplayContentScale(SDL_DisplayID)   { return 1.f; }
/*
    SDL_GetDisplayBounds(SDL_DisplayID, SDL_Rect* rect):
    - Params:   SDL_DisplayID, SDL_Rect* rect
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool          SDL_GetDisplayBounds(SDL_DisplayID, SDL_Rect* rect) {
    if (rect) *rect = SDL_Rect{0, 0, 0, 0};
    return false;
}
/*
    SDL_GetCurrentDisplayMode(SDL_DisplayID):
    - Params:   SDL_DisplayID
    - Returns:  const SDL_DisplayMode*
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
const SDL_DisplayMode* SDL_GetCurrentDisplayMode(SDL_DisplayID) { return nullptr; }
/*
    SDL_GetDisplays(int* count):
    - Params:   int* count
    - Returns:  SDL_DisplayID*
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
SDL_DisplayID* SDL_GetDisplays(int* count) {
    if (count) *count = 0;
    return nullptr;
}
/*
    SDL_GetSystemTheme():
    - Params:   none
    - Returns:  SDL_SystemTheme
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
SDL_SystemTheme SDL_GetSystemTheme()      { return SDL_SYSTEM_THEME_UNKNOWN; }
/*
    SDL_GetNumLogicalCPUCores():
    - Params:   none
    - Returns:  int
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
int SDL_GetNumLogicalCPUCores()           { return 0; }
/*
    SDL_GetSystemRAM():
    - Params:   none
    - Returns:  int
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
int SDL_GetSystemRAM()                    { return 0; }
/*
    SDL_GetBasePath():
    - Params:   none
    - Returns:  const char*
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
const char* SDL_GetBasePath()             { return ""; }
/*
    SDL_GetPrefPath(const char*, const char*):
    - Params:   const char*, const char*
    - Returns:  char*
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
char* SDL_GetPrefPath(const char*, const char*) { return nullptr; }

/*
    SDL_GetWindowSize(SDL_Window*, int* w, int* h):
    - Params:   SDL_Window*, int* w, int* h
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool SDL_GetWindowSize(SDL_Window*, int* w, int* h) {
    if (w) *w = 0;
    if (h) *h = 0;
    return true;
}

/*
    SDL_GetWindowSizeInPixels(SDL_Window*, int* w, int* h):
    - Params:   SDL_Window*, int* w, int* h
    - Returns:  bool
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
bool SDL_GetWindowSizeInPixels(SDL_Window*, int* w, int* h) {
    if (w) *w = 0;
    if (h) *h = 0;
    return true;
}

/*
    SDL_GetWindowProperties(SDL_Window*):
    - Params:   SDL_Window*
    - Returns:  SDL_PropertiesID
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
SDL_PropertiesID SDL_GetWindowProperties(SDL_Window*) { return 0; }

/*
    SDL_GetPointerProperty(SDL_PropertiesID, const char*, void* default_value):
    - Params:   SDL_PropertiesID, const char*, void* default_value
    - Returns:  void*
    - Desc:     Stub for the shim's declaration of this SDL entry point, which
                the web build never reaches.
*/
void* SDL_GetPointerProperty(SDL_PropertiesID, const char*, void* default_value) {
    return default_value;
}

namespace uilo {

/*
    configureMacWindowForLiveResize(void*):
    - Params:   void*
    - Returns:  bool
    - Desc:     Stub for the macOS window shim; a headless build has no window.
*/
bool configureMacWindowForLiveResize(void*) { return false; }
/*
    installMacScrollMonitor(std::function<bool(float, float, bool)>):
    - Params:   std::function<bool(float, float, bool)>
    - Returns:  bool
    - Desc:     Stub for the macOS input shim, which a headless build never
                installs.
*/
bool installMacScrollMonitor(std::function<bool(float, float, bool)>) { return false; }
/*
    installMacZoomMonitor(std::function<bool(float)>):
    - Params:   std::function<bool(float)>
    - Returns:  bool
    - Desc:     Stub for the macOS input shim, which a headless build never
                installs.
*/
bool installMacZoomMonitor(std::function<bool(float)>) { return false; }
/*
    tickMacScrollMomentum(float):
    - Params:   float
    - Returns:  none
    - Desc:     Stub for the macOS input shim, which a headless build never
                installs.
*/
void tickMacScrollMomentum(float) {}
/*
    cancelMacScrollMomentum():
    - Params:   none
    - Returns:  none
    - Desc:     Stub for the macOS input shim, which a headless build never
                installs.
*/
void cancelMacScrollMomentum() {}

/* Renderer */

/* Renderer holds a unique_ptr<Impl>, so Impl has to be complete where the
   destructor is emitted. Empty is enough -- there is no GPU state to keep. */
struct Renderer::Impl {};

/*
    Renderer():
    - Params:   none
    - Returns:  R
    - Desc:     No-op in the headless build, which never draws.
*/
Renderer::Renderer()  = default;
/*
    ~Renderer():
    - Params:   none
    - Returns:  R
    - Desc:     No-op in the headless build, which never draws.
*/
Renderer::~Renderer() = default;

/*
    init(uint32_t width, uint32_t height, const std::string&, uint8_t msaa):
    - Params:   uint32_t width, uint32_t height, const std::string&, uint8_t
                msaa
    - Returns:  bool
    - Desc:     No-op in the headless build, which never draws.
*/
bool Renderer::init(uint32_t width, uint32_t height, const std::string&, uint8_t msaa) {
    m_lastWidth   = width;
    m_lastHeight  = height;
    m_msaa        = msaa;
    m_initialised = true;
    return true;
}

/*
    attach(SDL_Window*, uint16_t):
    - Params:   SDL_Window*, uint16_t
    - Returns:  bool
    - Desc:     No-op in the headless build, which never draws.
*/
bool Renderer::attach(SDL_Window*, uint16_t) {
    m_ownsContext = false;
    m_initialised = true;
    return true;
}

/*
    shutdown():
    - Params:   none
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::shutdown()   { m_initialised = false; }
/*
    beginFrame():
    - Params:   none
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::beginFrame() {}
/*
    endFrame():
    - Params:   none
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::endFrame()   {}

/*
    getSize():
    - Params:   none
    - Returns:  Vec2u
    - Desc:     No-op in the headless build, which never draws.
*/
Vec2u Renderer::getSize() const { return { m_lastWidth, m_lastHeight }; }

/*
    setTitle(const std::string&):
    - Params:   const std::string&
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void  Renderer::setTitle(const std::string&) {}
/*
    setVsync(bool):
    - Params:   bool
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void  Renderer::setVsync(bool) {}
/*
    getVsync():
    - Params:   none
    - Returns:  bool
    - Desc:     No-op in the headless build, which never draws.
*/
bool  Renderer::getVsync() const { return false; }
/*
    setFramerateLimit(float):
    - Params:   float
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void  Renderer::setFramerateLimit(float) {}
/*
    getFramerateLimit():
    - Params:   none
    - Returns:  float
    - Desc:     No-op in the headless build, which never draws.
*/
float Renderer::getFramerateLimit() const { return 0.f; }

/*
    getStats():
    - Params:   none
    - Returns:  RendererStats
    - Desc:     No-op in the headless build, which never draws.
*/
RendererStats Renderer::getStats() const { return {}; }

/*
    setCursor(CursorType):
    - Params:   CursorType
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::setCursor(CursorType) {}

/*
    draw(const Rect&):
    - Params:   const Rect&
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::draw(const Rect&)        {}
/*
    draw(const RoundedRect&):
    - Params:   const RoundedRect&
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::draw(const RoundedRect&) {}
/*
    draw(const Circle&):
    - Params:   const Circle&
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::draw(const Circle&)      {}
/*
    draw(const Triangle&):
    - Params:   const Triangle&
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::draw(const Triangle&)    {}
/*
    draw(const Line&):
    - Params:   const Line&
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::draw(const Line&)        {}

/*
    drawLines(const Line*, size_t):
    - Params:   const Line*, size_t
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::drawLines(const Line*, size_t) {}
/*
    drawArc(Vec2f, float, float, float, float, Color, int):
    - Params:   Vec2f, float, float, float, float, Color, int
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::drawArc(Vec2f, float, float, float, float, Color, int) {}

/*
    loadTexture(const std::string&):
    - Params:   const std::string&
    - Returns:  Texture
    - Desc:     No-op in the headless build, which never draws.
*/
Texture Renderer::loadTexture(const std::string&) { return {}; }
/*
    destroyTexture(Texture& tex):
    - Params:   Texture& tex
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void    Renderer::destroyTexture(Texture& tex)    { tex = Texture{}; }

/*
    loadImagePixels(const std::string&, std::vector<uint8_t>&, uint32_t&, uint32_t&):
    - Params:   const std::string&, std::vector<uint8_t>&, uint32_t&, uint32_t&
    - Returns:  bool
    - Desc:     No-op in the headless build, which never draws.
*/
bool Renderer::loadImagePixels(const std::string&, std::vector<uint8_t>&,
                               uint32_t&, uint32_t&) {
    return false;
}

/*
    createTexture(uint16_t, uint16_t):
    - Params:   uint16_t, uint16_t
    - Returns:  Texture
    - Desc:     No-op in the headless build, which never draws.
*/
Texture Renderer::createTexture(uint16_t, uint16_t) { return {}; }
/*
    updateTexture(const Texture&, const uint8_t*):
    - Params:   const Texture&, const uint8_t*
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void    Renderer::updateTexture(const Texture&, const uint8_t*) {}

/*
    drawImage(const Rectf&, const Texture&, Color, Rectf, bool, bool, bool):
    - Params:   const Rectf&, const Texture&, Color, Rectf, bool, bool, bool
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::drawImage(const Rectf&, const Texture&, Color, Rectf, bool, bool, bool) {}

/*
    drawGlass(const Rectf&, const Material&, Color):
    - Params:   const Rectf&, const Material&, Color
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::drawGlass(const Rectf&, const Material&, Color) {}
/*
    setMouseState(Vec2f pos):
    - Params:   Vec2f pos
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::setMouseState(Vec2f pos) { m_mousePos = pos; }

/*
    loadFont(const std::string&):
    - Params:   const std::string&
    - Returns:  Font
    - Desc:     No-op in the headless build, which never draws.
*/
Font Renderer::loadFont(const std::string&) { return {}; }

/*
    drawText(const std::string&, Vec2f, const Font&, float, Color, TextStyle):
    - Params:   const std::string&, Vec2f, const Font&, float, Color, TextStyle
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::drawText(const std::string&, Vec2f, const Font&, float, Color, TextStyle) {}

/*
    measureText(const std::string&, const Font&, float):
    - Params:   const std::string&, const Font&, float
    - Returns:  TextMetrics
    - Desc:     No-op in the headless build, which never draws.
*/
TextMetrics Renderer::measureText(const std::string&, const Font&, float) { return {}; }

/*
    charPositions(const std::string&, const Font&, float):
    - Params:   const std::string&, const Font&, float
    - Returns:  std::vector<Vec2f>
    - Desc:     No-op in the headless build, which never draws.
*/
std::vector<Vec2f> Renderer::charPositions(const std::string&, const Font&, float) {
    return { Vec2f{0.f, 0.f} };
}

/*
    createFrameBuffer(Vec2u size):
    - Params:   Vec2u size
    - Returns:  FrameBuffer
    - Desc:     No-op in the headless build, which never draws.
*/
FrameBuffer Renderer::createFrameBuffer(Vec2u size) {
    FrameBuffer fb;
    fb.size = size;
    return fb;
}

/*
    resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize):
    - Params:   FrameBuffer& fb, Vec2u newSize
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize) { fb.size = newSize; }
/*
    destroyFrameBuffer(FrameBuffer& fb):
    - Params:   FrameBuffer& fb
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::destroyFrameBuffer(FrameBuffer& fb)               { fb = FrameBuffer{}; }
/*
    pushFrameBuffer(FrameBuffer&):
    - Params:   FrameBuffer&
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::pushFrameBuffer(FrameBuffer&)                     {}
/*
    popFrameBuffer():
    - Params:   none
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::popFrameBuffer()                                  {}
/*
    drawFrameBuffer(const FrameBuffer&, Vec2f, Vec2f, Color):
    - Params:   const FrameBuffer&, Vec2f, Vec2f, Color
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::drawFrameBuffer(const FrameBuffer&, Vec2f, Vec2f, Color) {}

/*
    clear(Color):
    - Params:   Color
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::clear(Color) {}

/*
    pushScissor(Rectf):
    - Params:   Rectf
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::pushScissor(Rectf) {}
/*
    popScissor():
    - Params:   none
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::popScissor()       {}

/*
    pushRoundClip(Rectf, float):
    - Params:   Rectf, float
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::pushRoundClip(Rectf, float) {}
/*
    popRoundClip():
    - Params:   none
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::popRoundClip()              {}

/*
    beginGlassSubtree():
    - Params:   none
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::beginGlassSubtree() {}
/*
    endGlassSubtree():
    - Params:   none
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::endGlassSubtree()   {}

/*
    setRotation(float, Vec2f):
    - Params:   float, Vec2f
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::setRotation(float, Vec2f) {}
/*
    rotate(float):
    - Params:   float
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::rotate(float)             {}
/*
    clearRotation():
    - Params:   none
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void Renderer::clearRotation()           {}

/*
    currentViewId():
    - Params:   none
    - Returns:  uint16_t
    - Desc:     No-op in the headless build, which never draws.
*/
uint16_t Renderer::currentViewId() const { return 0; }
/*
    submitOrtho(uint16_t, Vec2u):
    - Params:   uint16_t, Vec2u
    - Returns:  none
    - Desc:     No-op in the headless build, which never draws.
*/
void     Renderer::submitOrtho(uint16_t, Vec2u) {}


/*
    Pty:
    - Desc: Stubs for the web build, where a page has no shell to attach to --
            a browser cannot fork a process, and proxying one to the server
            would hand every visitor a shell on the host. Terminal is compiled
            for the web because it lives under elements/, so it needs these to
            link; with them it renders as an empty screen and reports that no
            shell could be started, which is the honest outcome.
    - Serving a real shell to a browser would need an explicit, authenticated
      server-side channel. That is a deliberate decision for whoever deploys
      the app, not something UILO should do implicitly.
*/
Pty::~Pty() {}

bool Pty::open(const std::string&, int, int) {
    m_error = "pty: a shell cannot be attached in the web build";
    return false;
}
void        Pty::close()                         {}
std::size_t Pty::read(std::string&)              { return 0; }
void        Pty::write(const char*, std::size_t) {}
void        Pty::resize(int, int)                {}
bool        Pty::childExited()                   { return true; }

} // namespace uilo
