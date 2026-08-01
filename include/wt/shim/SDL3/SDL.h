// SDL3 stand-in for UILO's headless (UILO_WT) build.
//
// UILO reaches for SDL in exactly two ways: a handful of input-polling calls
// inside the interactible widgets, and the event pump in UILO.cpp. Neither
// runs on a web server -- the browser is the input device there, and the Wt
// bridge drives widgets through their public setters instead.
//
// Rather than patch eight `#include <SDL3/SDL.h>` lines across the tree, the
// UILO_WT build puts this directory first on the include path so those
// includes resolve here. UILO's sources compile untouched; the functions are
// defined as no-ops in wt/HeadlessBackend.cpp.
//
// Values mirror real SDL3 so that a file compiled against this header and one
// compiled against the real SDL agree on every constant.

#ifndef UILO_WT_SDL3_SHIM_H
#define UILO_WT_SDL3_SHIM_H

#include <cstdint>

using Uint8  = uint8_t;
using Uint16 = uint16_t;
using Uint32 = uint32_t;
using Uint64 = uint64_t;
using Sint32 = int32_t;

// Opaque handles. `struct SDL_Window;` matches the forward declaration in
// UILO's Renderer.hpp, so both spellings refer to the same incomplete type.
struct SDL_Window;
struct SDL_Cursor;

using SDL_Keycode     = Uint32;
using SDL_Keymod      = Uint16;
using SDL_PropertiesID = Uint32;
using SDL_MouseButtonFlags = Uint32;
using SDL_DisplayID   = Uint32;
using SDL_InitFlags   = Uint32;

// Physical key positions, used by uilo::Keybinds. Real SDL3 declares this as an
// enum; a plain integer alias is enough here and still lets application code
// name any SDL_SCANCODE_* constant it defines below.
using SDL_Scancode = Uint32;

// ---- Mouse ---------------------------------------------------------------
#define SDL_BUTTON_LEFT   1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT  3
#define SDL_BUTTON_MASK(X) (1u << ((X) - 1))

// ---- Key modifiers -------------------------------------------------------
#define SDL_KMOD_NONE   0x0000u
#define SDL_KMOD_LSHIFT 0x0001u
#define SDL_KMOD_RSHIFT 0x0002u
#define SDL_KMOD_SHIFT  (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT)
#define SDL_KMOD_LCTRL  0x0040u
#define SDL_KMOD_RCTRL  0x0080u
#define SDL_KMOD_CTRL   (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL)
#define SDL_KMOD_LGUI   0x0400u
#define SDL_KMOD_RGUI   0x0800u
#define SDL_KMOD_GUI    (SDL_KMOD_LGUI | SDL_KMOD_RGUI)

// ---- Keycodes ------------------------------------------------------------
// Printable keys are their own codepoint; the rest are scancodes tagged with
// SDLK_SCANCODE_MASK, exactly as SDL3 defines them.
#define SDLK_SCANCODE_MASK (1u << 30)
#define SDL_SCANCODE_TO_KEYCODE(X) ((X) | SDLK_SCANCODE_MASK)

#define SDLK_BACKSPACE 0x00000008u
#define SDLK_TAB       0x00000009u
#define SDLK_RETURN    0x0000000du
#define SDLK_ESCAPE    0x0000001bu
#define SDLK_MINUS     0x0000002du
#define SDLK_EQUALS    0x0000003du
#define SDLK_DELETE    0x0000007fu
#define SDLK_A         0x00000061u
#define SDLK_C         0x00000063u
#define SDLK_V         0x00000076u
#define SDLK_X         0x00000078u

#define SDLK_HOME      SDL_SCANCODE_TO_KEYCODE(74u)
#define SDLK_PAGEUP    SDL_SCANCODE_TO_KEYCODE(75u)
#define SDLK_END       SDL_SCANCODE_TO_KEYCODE(77u)
#define SDLK_PAGEDOWN  SDL_SCANCODE_TO_KEYCODE(78u)
#define SDLK_RIGHT     SDL_SCANCODE_TO_KEYCODE(79u)
#define SDLK_LEFT      SDL_SCANCODE_TO_KEYCODE(80u)
#define SDLK_DOWN      SDL_SCANCODE_TO_KEYCODE(81u)
#define SDLK_UP        SDL_SCANCODE_TO_KEYCODE(82u)
#define SDLK_KP_MINUS  SDL_SCANCODE_TO_KEYCODE(86u)
#define SDLK_KP_PLUS   SDL_SCANCODE_TO_KEYCODE(87u)
#define SDLK_KP_ENTER  SDL_SCANCODE_TO_KEYCODE(88u)

// ---- Scancodes -----------------------------------------------------------
// Same numbering as real SDL3, so a binding compiled against either header
// refers to the same physical key.
#define SDL_SCANCODE_A 4u
#define SDL_SCANCODE_B 5u
#define SDL_SCANCODE_C 6u
#define SDL_SCANCODE_D 7u
#define SDL_SCANCODE_E 8u
#define SDL_SCANCODE_F 9u
#define SDL_SCANCODE_G 10u
#define SDL_SCANCODE_H 11u
#define SDL_SCANCODE_I 12u
#define SDL_SCANCODE_J 13u
#define SDL_SCANCODE_K 14u
#define SDL_SCANCODE_L 15u
#define SDL_SCANCODE_M 16u
#define SDL_SCANCODE_N 17u
#define SDL_SCANCODE_O 18u
#define SDL_SCANCODE_P 19u
#define SDL_SCANCODE_Q 20u
#define SDL_SCANCODE_R 21u
#define SDL_SCANCODE_S 22u
#define SDL_SCANCODE_T 23u
#define SDL_SCANCODE_U 24u
#define SDL_SCANCODE_V 25u
#define SDL_SCANCODE_W 26u
#define SDL_SCANCODE_X 27u
#define SDL_SCANCODE_Y 28u
#define SDL_SCANCODE_Z 29u

#define SDL_SCANCODE_1 30u
#define SDL_SCANCODE_2 31u
#define SDL_SCANCODE_3 32u
#define SDL_SCANCODE_4 33u
#define SDL_SCANCODE_5 34u
#define SDL_SCANCODE_6 35u
#define SDL_SCANCODE_7 36u
#define SDL_SCANCODE_8 37u
#define SDL_SCANCODE_9 38u
#define SDL_SCANCODE_0 39u

#define SDL_SCANCODE_RETURN    40u
#define SDL_SCANCODE_ESCAPE    41u
#define SDL_SCANCODE_BACKSPACE 42u
#define SDL_SCANCODE_TAB       43u
#define SDL_SCANCODE_SPACE     44u
#define SDL_SCANCODE_MINUS     45u
#define SDL_SCANCODE_EQUALS    46u

#define SDL_SCANCODE_F1  58u
#define SDL_SCANCODE_F2  59u
#define SDL_SCANCODE_F3  60u
#define SDL_SCANCODE_F4  61u
#define SDL_SCANCODE_F5  62u
#define SDL_SCANCODE_F6  63u
#define SDL_SCANCODE_F7  64u
#define SDL_SCANCODE_F8  65u
#define SDL_SCANCODE_F9  66u
#define SDL_SCANCODE_F10 67u
#define SDL_SCANCODE_F11 68u
#define SDL_SCANCODE_F12 69u

#define SDL_SCANCODE_HOME     74u
#define SDL_SCANCODE_PAGEUP   75u
#define SDL_SCANCODE_DELETE   76u
#define SDL_SCANCODE_END      77u
#define SDL_SCANCODE_PAGEDOWN 78u
#define SDL_SCANCODE_RIGHT    79u
#define SDL_SCANCODE_LEFT     80u
#define SDL_SCANCODE_DOWN     81u
#define SDL_SCANCODE_UP       82u

#define SDL_SCANCODE_KP_MINUS 86u
#define SDL_SCANCODE_KP_PLUS  87u

// ---- Events --------------------------------------------------------------
#define SDL_EVENT_QUIT                     0x100u
#define SDL_EVENT_WINDOW_RESIZED           0x204u
#define SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED 0x205u
#define SDL_EVENT_KEY_DOWN                 0x300u
#define SDL_EVENT_KEY_UP                   0x301u
#define SDL_EVENT_TEXT_INPUT               0x303u
#define SDL_EVENT_MOUSE_WHEEL              0x403u

struct SDL_CommonEvent {
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
};

struct SDL_KeyboardEvent {
    Uint32      type;
    Uint32      reserved;
    Uint64      timestamp;
    SDL_Keycode key;
    SDL_Keymod  mod;
    bool        down;
    bool        repeat;
};

struct SDL_TextInputEvent {
    Uint32      type;
    Uint32      reserved;
    Uint64      timestamp;
    const char* text;
};

struct SDL_MouseWheelEvent {
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
    float  x;
    float  y;
};

union SDL_Event {
    Uint32              type;
    SDL_CommonEvent     common;
    SDL_KeyboardEvent   key;
    SDL_TextInputEvent  text;
    SDL_MouseWheelEvent wheel;
    Uint8               padding[128];
};

using SDL_EventFilter = bool (*)(void* userdata, SDL_Event* event);

// ---- Display / system (used by uilo::OS) ---------------------------------
#define SDL_INIT_VIDEO 0x00000020u

struct SDL_Rect {
    int x, y, w, h;
};

struct SDL_DisplayMode {
    SDL_DisplayID displayID;
    Uint32        format;
    int           w;
    int           h;
    float         pixel_density;
    float         refresh_rate;
    int           refresh_rate_numerator;
    int           refresh_rate_denominator;
};

enum SDL_SystemTheme {
    SDL_SYSTEM_THEME_UNKNOWN = 0,
    SDL_SYSTEM_THEME_LIGHT   = 1,
    SDL_SYSTEM_THEME_DARK    = 2,
};

// ---- Window properties ---------------------------------------------------
#define SDL_PROP_WINDOW_COCOA_WINDOW_POINTER "SDL.window.cocoa.window"

// ---- Functions (defined as no-ops in wt/HeadlessBackend.cpp) --------------
SDL_MouseButtonFlags SDL_GetMouseState(float* x, float* y);
Uint64      SDL_GetTicks();
SDL_Keymod  SDL_GetModState();
const bool* SDL_GetKeyboardState(int* numkeys);

char* SDL_GetClipboardText();
bool  SDL_SetClipboardText(const char* text);
void  SDL_free(void* mem);

bool SDL_StartTextInput(SDL_Window* window);
bool SDL_StopTextInput(SDL_Window* window);
bool SDL_TextInputActive(SDL_Window* window);

bool SDL_AddEventWatch(SDL_EventFilter filter, void* userdata);
bool SDL_PollEvent(SDL_Event* event);

// Relative mouse mode / motion, used by uilo::Mousebinds. There is no cursor to
// capture on a web server, so these report "off" and zero motion.
bool  SDL_SetWindowRelativeMouseMode(SDL_Window* window, bool enabled);
bool  SDL_GetWindowRelativeMouseMode(SDL_Window* window);
void  SDL_GetRelativeMouseState(float* x, float* y);
float SDL_GetWindowDisplayScale(SDL_Window* window);
const char* SDL_GetError();

// Display / system queries behind uilo::OS. A web server has no local display,
// so these report "unknown" and OS falls back to its documented defaults.
SDL_InitFlags SDL_WasInit(SDL_InitFlags flags);
SDL_DisplayID SDL_GetPrimaryDisplay();
float         SDL_GetDisplayContentScale(SDL_DisplayID displayID);
bool          SDL_GetDisplayBounds(SDL_DisplayID displayID, SDL_Rect* rect);
const SDL_DisplayMode* SDL_GetCurrentDisplayMode(SDL_DisplayID displayID);
SDL_DisplayID* SDL_GetDisplays(int* count);
SDL_SystemTheme SDL_GetSystemTheme();
int   SDL_GetNumLogicalCPUCores();
int   SDL_GetSystemRAM();
const char* SDL_GetBasePath();
char* SDL_GetPrefPath(const char* org, const char* app);

bool SDL_GetWindowSize(SDL_Window* window, int* w, int* h);
bool SDL_GetWindowSizeInPixels(SDL_Window* window, int* w, int* h);

SDL_PropertiesID SDL_GetWindowProperties(SDL_Window* window);
void* SDL_GetPointerProperty(SDL_PropertiesID props, const char* name,
                             void* default_value);

#endif // UILO_WT_SDL3_SHIM_H
