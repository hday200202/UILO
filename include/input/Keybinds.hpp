#pragma once

#include <SDL3/SDL.h>
#include <functional>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace uilo {

/*
    Keybinds
    - Desc: Maps a semantic action name to one or more physical keys, polled
            once per frame rather than driven by events. Binding by scancode
            (physical position) instead of keycode keeps a binding on the same
            key across keyboard layouts.
    - bindAction(name, key(s), callback, onPressOnly)
        onPressOnly = true  -> fires once on the press edge
        onPressOnly = false -> fires every frame the key is held
      Several keys bound to one action behave as "any of these", so
      { SDL_SCANCODE_EQUALS, SDL_SCANCODE_KP_PLUS } is a single action.
    - update() is driven by UILO::update(); an application only binds and
      unbinds. While a focused widget is consuming text input, UILO calls it
      with dispatch=false so typing never triggers application shortcuts.
*/
class Keybinds {
public:
    using ActionCallback = std::function<void()>;

    struct Action {
        std::string               name;
        std::vector<SDL_Scancode> scancodes;
        ActionCallback            callback;
        bool                      onPressOnly = false;
    };

    void bindAction(
        const std::string& name,
        std::initializer_list<SDL_Scancode> scancodes,
        ActionCallback callback,
        bool onPressOnly = false
    ) {
        m_actions[name] = { name, scancodes, std::move(callback), onPressOnly };
        for (auto scancode : scancodes) m_prevState.try_emplace(scancode, false);
    }

    void bindAction(
        const std::string& name,
        SDL_Scancode scancode,
        ActionCallback callback,
        bool onPressOnly = false
    ) {
        bindAction(name, { scancode }, std::move(callback), onPressOnly);
    }

    void unbindAction(const std::string& name) { m_actions.erase(name); }
    void clear()                               { m_actions.clear(); m_prevState.clear(); }

    bool hasAction(const std::string& name) const {
        return m_actions.find(name) != m_actions.end();
    }

    /* True while any key bound to the action is held. */
    bool isActionDown(const std::string& name) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        const bool* state = SDL_GetKeyboardState(nullptr);
        for (auto scancode : it->second.scancodes)
            if (state[scancode]) return true;
        return false;
    }

    const std::unordered_map<std::string, Action>& getActions() const { return m_actions; }

    /* dispatch=false advances the edge-detection state without firing any
       callback, so a key held while suppressed is not reported as a fresh. */
    void update(bool dispatch = true) {
        const bool* state = SDL_GetKeyboardState(nullptr);

        if (dispatch) {
            for (auto& [name, action] : m_actions) {
                (void)name;
                bool isDown  = false;
                bool wasDown = false;

                for (auto scancode : action.scancodes) {
                    if (state[scancode]) isDown = true;
                    auto prev = m_prevState.find(scancode);
                    if (prev != m_prevState.end() && prev->second) wasDown = true;
                }

                if (isDown && action.callback && (!action.onPressOnly || !wasDown))
                    action.callback();
            }
        }

        /* Snapshot after every action has been evaluated. */
        for (auto& [scancode, prev] : m_prevState) prev = state[scancode];
    }

private:
    std::unordered_map<std::string, Action>  m_actions;
    std::unordered_map<SDL_Scancode, bool>   m_prevState;
};

} // namespace uilo
