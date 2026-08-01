#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "Math.hpp"

namespace uilo {

/*
    OSFamily
    - Which operating system the binary is running on. Resolved at compile time,
      so it is usable in constant expressions and costs nothing at runtime.
*/
enum class OSFamily {
    Unknown, MacOS, Windows, Linux, BSD, Android, IOS, Web,
};

/*
    OSTheme
    - The system-wide light/dark preference, when the platform reports one.
*/
enum class OSTheme { Unknown, Light, Dark };


/*
    OS
    - Desc: Information about the machine UILO is running on -- the counterpart
            to Resources, which covers assets. Static queries only; there is
            nothing to construct.
    - The point of scale(): instead of hard-coding a magnification, hand the
      platform's own content scale to UILO and the interface comes out the
      intended physical size on any display.

            ui.setScale(OS::scale());

      That is 1.0 on a standard display and on macOS (where the OS composites
      retina itself), and 1.5 or 2.0 on a Windows/Linux desktop set to 150% or
      200%. Note the parentheses: scale is a live query, not a constant, because
      it depends on the display the app opened on and can change when the window
      moves between monitors.
    - Identity queries (family, name, architecture) need nothing initialised.
      Everything under "display" and "system" reads through SDL and is only
      meaningful once the renderer is up; before that they return the documented
      fallbacks rather than failing.
*/
class OS {
public:
    OS() = delete;

    /* Identity (compile-time) */
    static constexpr OSFamily family();
    /* "macOS", "Windows", "Linux", "Android", "iOS", "FreeBSD", "Web". */
    static constexpr std::string_view name();
    /* "arm64", "x86_64", "x86", "arm", or "unknown". */
    static constexpr std::string_view architecture();
    static constexpr bool is64Bit();

    static constexpr bool isMacOS()   { return family() == OSFamily::MacOS; }
    static constexpr bool isWindows() { return family() == OSFamily::Windows; }
    static constexpr bool isLinux()   { return family() == OSFamily::Linux; }
    static constexpr bool isApple()   { return family() == OSFamily::MacOS || family() == OSFamily::IOS; }
    static constexpr bool isMobile()  { return family() == OSFamily::Android || family() == OSFamily::IOS; }
    static constexpr bool isWeb()     { return family() == OSFamily::Web; }
    static constexpr bool isDesktop();

    /* Display. */
    /* Real panel pixels per virtual pixel: physicalDisplaySize() divided by. */
    static float scale();
    /* Primary display in virtual points -- the space window sizes and mouse
       positions live in. 1470x956 in the example above. */
    static Vec2u displaySize();
    /* Native pixel resolution of the panel itself: 2560x1664 in the example
       above. Zero if it cannot be determined. */
    static Vec2u physicalDisplaySize();
    /* Backing-store pixels per virtual point, as the OS reports it: 2.0 on a
       retina panel. */
    static float pixelDensity();
    static float refreshRate();
    static int   displayCount();

    /* System */
    static OSTheme theme();
    static bool prefersDarkMode() { return theme() == OSTheme::Dark; }
    /* Logical cores, including SMT. 0 if unknown. */
    static int cpuCount();
    /* Total system RAM in MiB. 0 if unknown. */
    static int systemRamMB();

    /* Paths. */
    /* Directory the executable lives in, with a trailing separator. */
    static std::string executableDirectory();
    /* Per-user, per-application directory for writable state, created if
       missing. Empty on failure. */
    static std::string preferencesDirectory(const std::string& organization,
                                            const std::string& application);
};


/* Compile-time identity */
constexpr OSFamily OS::family() {
#if defined(__EMSCRIPTEN__)
    return OSFamily::Web;
#elif defined(__ANDROID__)   /* must precede __linux__, which it also defines */
    return OSFamily::Android;
#elif defined(__APPLE__)
#  if defined(UILO_OS_IPHONE)
    return OSFamily::IOS;
#  else
    return OSFamily::MacOS;
#  endif
#elif defined(_WIN32) || defined(_WIN64)
    return OSFamily::Windows;
#elif defined(__linux__)
    return OSFamily::Linux;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    return OSFamily::BSD;
#else
    return OSFamily::Unknown;
#endif
}

constexpr std::string_view OS::name() {
    switch (family()) {
        case OSFamily::MacOS:   return "macOS";
        case OSFamily::Windows: return "Windows";
        case OSFamily::Linux:   return "Linux";
        case OSFamily::BSD:     return "BSD";
        case OSFamily::Android: return "Android";
        case OSFamily::IOS:     return "iOS";
        case OSFamily::Web:     return "Web";
        default:                return "Unknown";
    }
}

constexpr std::string_view OS::architecture() {
#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64__)
    return "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#elif defined(__wasm__) || defined(__wasm32__)
    return "wasm";
#else
    return "unknown";
#endif
}

constexpr bool OS::is64Bit() {
    return sizeof(void*) == 8;
}

constexpr bool OS::isDesktop() {
    switch (family()) {
        case OSFamily::MacOS:
        case OSFamily::Windows:
        case OSFamily::Linux:
        case OSFamily::BSD:   return true;
        default:              return false;
    }
}

} // namespace uilo
