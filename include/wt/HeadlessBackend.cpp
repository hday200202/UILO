/*
    HeadlessBackend.cpp:
    - Desc: The whole GPU and OS-input surface, stubbed out, for the UILO_WT
            build. The Wt bridge builds a UILO element tree and reads it but
            never ticks or draws it, so every one of these entry points can
            collapse to a no-op and VALO is then neither compiled nor linked.
            Renderer.hpp only forward-declares VALO's window, so the header
            stays usable unchanged.
    - Four groups live here: the entire Renderer class, the valo::Window
      members UILO's input code names (its key and button types come from
      VALO's header-only render/Window.hpp, but the members need definitions to
      link), the macOS trackpad and live-resize shims UILO.cpp calls -- UILO's
      own MacStubs.cpp is guarded on !__APPLE__, so a headless macOS build would
      otherwise be missing them -- and Pty, which Terminal links against but
      which cannot exist on the web at all.
    - Nothing here is on a path the web app reaches. Renderer::window() is null
      here, and every caller checks for that, so a headless program that does
      call update() or render() draws nothing rather than crashing.
*/

#include "../renderer/Renderer.hpp"
#include "../platform/MacScroll.hpp"
#include "../platform/MacWindow.hpp"
#include "../platform/Pty.hpp"

#include <render/Window.hpp>

/*
    valo::Window:
    - Desc: Link-only stand-ins. No Window is ever constructed in the web build
            -- Renderer::window() returns null and every caller checks -- so none
            of these runs; they exist because UILO's input code names them.
*/
namespace valo {

bool  Window::pollEvents()                          { return false; }
bool  Window::isKeyDown(const Key) const            { return false; }
bool  Window::isMouseDown(const MouseButton) const  { return false; }
Vec2f Window::mousePosition() const                 { return {0.f, 0.f}; }
KeyMod Window::modState() const                     { return KeyMod::None; }
void  Window::grabMouse()                           {}
void  Window::releaseMouse()                        {}
bool  Window::isMouseGrabbed() const                { return false; }
float Window::getPixelDensity() const               { return 1.f; }
float Window::getDisplayScale() const               { return 1.f; }
void  Window::startTextInput()                      {}
void  Window::stopTextInput()                       {}
bool  Window::isTextInputActive() const             { return false; }
std::string Window::getClipboardText() const        { return {}; }
void  Window::setClipboardText(const std::string&)  {}
void  Window::setResizeCallback(std::function<void()>) {}
void* Window::nativeHandle() const                  { return nullptr; }

bool        Window::isVideoReady()                  { return false; }
float       Window::primaryDisplayContentScale()    { return 1.f; }
Vec2u       Window::primaryDisplayBounds()          { return {0, 0}; }
float       Window::primaryDisplayPixelDensity()    { return 1.f; }
float       Window::primaryDisplayRefreshRate()     { return 0.f; }
int         Window::displayCount()                  { return 0; }
int         Window::cpuCount()                      { return 0; }
int         Window::systemRamMB()                   { return 0; }
SystemTheme Window::getSystemTheme()                { return SystemTheme::Unknown; }
std::string Window::basePath()                      { return {}; }
std::string Window::prefPath(const std::string&, const std::string&) { return {}; }

} // namespace valo

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
    window():
    - Params:   none
    - Returns:  valo::Window*
    - Desc:     Always null: there is no window in the web build.
*/
valo::Window* Renderer::window() const { return nullptr; }

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
