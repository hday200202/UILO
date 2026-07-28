#pragma once

#include <SDL3/SDL.h>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <initializer_list>

class Keybinds {
public:
    using ActionCallback = std::function<void()>;

    struct Action {
        std::string name;
        std::vector<SDL_Scancode> scancodes;
        ActionCallback callback;
        bool onPressOnly = false;
    };

    void bindAction(const std::string& name, std::initializer_list<SDL_Scancode> scancodes, ActionCallback callback, bool onPressOnly = false) {
        m_actions[name] = { name, scancodes, callback, onPressOnly };
        
        for (auto scancode : scancodes)
            m_prevState[scancode] = false;
    }

    void bindAction(const std::string& name, SDL_Scancode scancode, ActionCallback callback, bool onPressOnly = false) {
        bindAction(name, { scancode }, callback, onPressOnly);
    }

    void unbindAction(const std::string& name) {
        m_actions.erase(name);
    }

    void update() {
        const bool* state = SDL_GetKeyboardState(NULL);

        for (auto& [name, action] : m_actions) {
            bool isDown = false;
            bool wasDown = false;

            for (auto scancode : action.scancodes) {
                if (state[scancode]) isDown = true;
                if (m_prevState[scancode]) wasDown = true;
            }

            if (isDown && action.callback)
                if (!action.onPressOnly || !wasDown)
                    action.callback();

            for (auto scancode : action.scancodes)
                m_prevState[scancode] = state[scancode];
        }
    }

private:
    std::unordered_map<std::string, Action> m_actions;
    std::unordered_map<SDL_Scancode, bool> m_prevState;
};