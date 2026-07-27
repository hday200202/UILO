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

// ---- Events --------------------------------------------------------------
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

bool SDL_GetWindowSize(SDL_Window* window, int* w, int* h);
bool SDL_GetWindowSizeInPixels(SDL_Window* window, int* w, int* h);

SDL_PropertiesID SDL_GetWindowProperties(SDL_Window* window);
void* SDL_GetPointerProperty(SDL_PropertiesID props, const char* name,
                             void* default_value);

#endif // UILO_WT_SDL3_SHIM_H
