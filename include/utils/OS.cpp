#include "OS.hpp"

#include "../platform/MacWindow.hpp"

#include <SDL3/SDL.h>

namespace uilo {

namespace {

/*
    videoReady():
    - Params:   none
    - Returns:  bool
    - Desc:     Every display query needs the video subsystem. UILO's
                renderer initialises it, but OS is callable before that (and
                from tools that never open a window), so each query checks
                rather than returning nonsense.
*/
bool videoReady() {
    return SDL_WasInit(SDL_INIT_VIDEO) != 0;
}

} // namespace


/*
    scale():
    - Params:   none
    - Returns:  float
    - Desc:     UILO lays out in virtual pixels that map 1:1 onto the
                window's pixels, so the useful scale is "how many real panel
                pixels does one virtual pixel cover" -- native panel
                resolution divided by the virtual desktop size. This is NOT
                SDL_GetDisplayContentScale, which reports 1.0 on macOS
                however the desktop is scaled, and NOT the backing-store
                ratio either: at a 1470x956 desktop macOS renders 2940x1912
                and scans it out to a 2560x1664 panel, so the backing ratio
                says 2.0 while the panel only gives 1.74 physical pixels per
                point. Using 2.0 would draw everything ~15% too large.
*/
float OS::scale() {
    const Vec2u nativePx = physicalDisplaySize();
    const Vec2u virtualPx = displaySize();
    if (nativePx.x > 0 && virtualPx.x > 0)
        return static_cast<float>(nativePx.x) / static_cast<float>(virtualPx.x);

    /* No panel information (non-macOS, or queried before the video subsystem
       is up): fall back to the platform's own content scale, which is what. */
    if (!videoReady()) return 1.f;
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (display == 0) return 1.f;

    const float s = SDL_GetDisplayContentScale(display);
    /* SDL reports 0 on failure; a zero scale would collapse the whole UI. */
    return s > 0.f ? s : 1.f;
}

/*
    displaySize():
    - Params:   none
    - Returns:  Vec2u
    - Desc:     Prefer the platform's own answer; SDL's bounds agree with it
                on macOS but this keeps the two halves of the scale ratio
                coming from one source.
*/
Vec2u OS::displaySize() {
    uint32_t w = 0, h = 0;
    if (getVirtualDisplaySize(w, h)) return Vec2u{w, h};

    if (!videoReady()) return Vec2u{0u, 0u};
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (display == 0) return Vec2u{0u, 0u};

    SDL_Rect bounds{0, 0, 0, 0};
    if (!SDL_GetDisplayBounds(display, &bounds)) return Vec2u{0u, 0u};
    return Vec2u{ static_cast<uint32_t>(bounds.w), static_cast<uint32_t>(bounds.h) };
}

/*
    physicalDisplaySize():
    - Params:   none
    - Returns:  Vec2u
    - Desc:     The primary display's real panel resolution in pixels. macOS is
                asked directly, since its framebuffer is not the panel;
                everywhere else the framebuffer is the panel, so the virtual
                size times the reported pixel density is the best answer
                available.
*/
Vec2u OS::physicalDisplaySize() {
    uint32_t w = 0, h = 0;
    if (getNativeDisplayPixelSize(w, h)) return Vec2u{w, h};

    /* Elsewhere the framebuffer *is* the panel, so backing pixels are the best
       available answer: virtual size times the reported pixel density. */
    const Vec2u virtualPx = displaySize();
    if (virtualPx.x == 0) return Vec2u{0u, 0u};
    const float density = pixelDensity();
    return Vec2u{
        static_cast<uint32_t>(static_cast<float>(virtualPx.x) * density + 0.5f),
        static_cast<uint32_t>(static_cast<float>(virtualPx.y) * density + 0.5f)
    };
}

/*
    pixelDensity():
    - Params:   none
    - Returns:  float
    - Desc:     Backing pixels per virtual pixel on the primary display. Falls
                back to 1 whenever the video subsystem is not up or the display
                cannot be queried, so a caller never has to guard against 0.
*/
float OS::pixelDensity() {
    if (!videoReady()) return 1.f;
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (display == 0) return 1.f;

    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
    if (!mode || mode->pixel_density <= 0.f) return 1.f;
    return mode->pixel_density;
}

/*
    refreshRate():
    - Params:   none
    - Returns:  float
    - Desc:     Refresh rate of the primary display in Hz, 0 when it cannot be
                determined.
*/
float OS::refreshRate() {
    if (!videoReady()) return 0.f;
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (display == 0) return 0.f;

    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
    return mode ? mode->refresh_rate : 0.f;
}

/*
    displayCount():
    - Params:   none
    - Returns:  int
    - Desc:     How many displays are attached, 0 before the video subsystem is
                up.
*/
int OS::displayCount() {
    if (!videoReady()) return 0;
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    if (!displays) return 0;
    SDL_free(displays);
    return count;
}

/*
    theme():
    - Params:   none
    - Returns:  OSTheme
    - Desc:     Whether the desktop is set to a light or dark appearance, so an
                application can pick a matching palette without asking the user.
*/
OSTheme OS::theme() {
    if (!videoReady()) return OSTheme::Unknown;
    switch (SDL_GetSystemTheme()) {
        case SDL_SYSTEM_THEME_LIGHT: return OSTheme::Light;
        case SDL_SYSTEM_THEME_DARK:  return OSTheme::Dark;
        default:                     return OSTheme::Unknown;
    }
}

/*
    cpuCount():
    - Params:   none
    - Returns:  int
    - Desc:     Number of logical CPUs, for sizing a worker pool.
*/
int OS::cpuCount() {
    const int n = SDL_GetNumLogicalCPUCores();
    return n > 0 ? n : 0;
}

/*
    systemRamMB():
    - Params:   none
    - Returns:  int
    - Desc:     Installed system memory in megabytes.
*/
int OS::systemRamMB() {
    const int mb = SDL_GetSystemRAM();
    return mb > 0 ? mb : 0;
}

/*
    executableDirectory():
    - Params:   none
    - Returns:  std::string
    - Desc:     Owned by SDL and valid for the process lifetime -- do not
                free.
*/
std::string OS::executableDirectory() {
    const char* base = SDL_GetBasePath();
    return base ? std::string(base) : std::string{};
}

/*
    preferencesDirectory(...):
    - Params:   const std::string& organization, const std::string&
                application
    - Returns:  std::string
    - Desc:     The per-user directory an application should keep its settings
                in, created if it does not exist. The organisation and
                application names become the path components the platform
                expects, so the result is the conventional location on each one.
*/
std::string OS::preferencesDirectory(const std::string& organization,
                                     const std::string& application) {
    char* path = SDL_GetPrefPath(organization.c_str(), application.c_str());
    if (!path) return {};
    std::string result(path);
    SDL_free(path);   /* this one is a fresh allocation, unlike SDL_GetBasePath */
    return result;
}

} // namespace uilo
