#pragma once

#include <SDL3/SDL.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <initializer_list>

#include "../../utils/Log.hpp"

class Mousebinds {
public:
    using MotionAction = std::function<void(float xRelative, float yRelative)>;
    using ButtonCallback = std::function<void()>;

    struct MotionActionItem {
        std::string name;
        MotionAction callback;
    };

    struct ButtonActionItem {
        std::string name;
        std::vector<uint32_t> buttonMasks; // Holds the pre-calculated masks for all bound buttons
        ButtonCallback callback;
        bool onPressOnly = false;
    };

    Mousebinds() = default;

    void setRelativeMode(SDL_Window* window, bool enabled) {
        if (!SDL_SetWindowRelativeMouseMode(window, enabled)) {
            utils::log::error(
                "MOUSEBINDS", "SDL_SetWindowRelativeMouseMode({}) failed: {}",
                enabled, SDL_GetError()
            );
        }
    }

    bool getRelativeMode(SDL_Window* window) const { 
        return SDL_GetWindowRelativeMouseMode(window);
    }

    void bindMotion(const std::string& name, MotionAction callback) {
        m_motionActions[name] = { name, callback };
    }

    void unbindMotion(const std::string& name) {
        m_motionActions.erase(name);
    }

    // Overload 1: For multiple mouse buttons via initializer list -> {SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT}
    void bindButton(
        const std::string& name, 
        std::initializer_list<uint8_t> buttons, 
        ButtonCallback callback, 
        bool onPressOnly = false
    ) {
        std::vector<uint32_t> masks;
        masks.reserve(buttons.size());

        for (uint8_t button : buttons) {
            uint32_t mask = SDL_BUTTON_MASK(button);
            masks.push_back(mask);
            m_prevButtonState[mask] = false;
        }

        m_buttonActions[name] = { name, masks, callback, onPressOnly };
    }

    // Overload 2: For a single mouse button -> SDL_BUTTON_LEFT
    void bindButton(
        const std::string& name, 
        uint8_t button, 
        ButtonCallback callback, 
        bool onPressOnly = false
    ) {
        bindButton(name, { button }, callback, onPressOnly);
    }

    void unbindButton(const std::string& name) {
        m_buttonActions.erase(name);
    }

    void update(SDL_Window* window) {
        uint32_t currentButtons = SDL_GetMouseState(nullptr, nullptr);

        for (auto& [name, action] : m_buttonActions) {
            bool isDown = false;
            bool wasDown = false;

            // Trigger action if ANY of the bound mouse buttons meet criteria
            for (uint32_t mask : action.buttonMasks) {
                if ((currentButtons & mask) != 0) isDown = true;
                if (m_prevButtonState[mask]) wasDown = true;
            }

            if (isDown && action.callback) {
                if (!action.onPressOnly || !wasDown)
                    action.callback();
            }

            // Record edge history per individual physical mask
            for (uint32_t mask : action.buttonMasks) {
                m_prevButtonState[mask] = ((currentButtons & mask) != 0);
            }
        }

        const bool relativeMode = getRelativeMode(window);

        float dx = 0.0f;
        float dy = 0.0f;
        SDL_GetRelativeMouseState(&dx, &dy);

        const bool justLocked = relativeMode && !m_wasRelativeMode;
        m_wasRelativeMode = relativeMode;

        if (!relativeMode || justLocked) return;
        if (dx == 0.0f && dy == 0.0f) return;

        const float scale = SDL_GetWindowDisplayScale(window);
        if (scale > 0.0f) {
            dx /= scale;
            dy /= scale;
        }

        for (const auto& [name, action] : m_motionActions)
            if (action.callback) action.callback(dx, dy);
    }

private:
    std::unordered_map<std::string, MotionActionItem> m_motionActions;

    std::unordered_map<std::string, ButtonActionItem> m_buttonActions; // Keyed by semantic name string
    std::unordered_map<uint32_t, bool> m_prevButtonState;             // Keyed by raw bitmask indices
    bool m_wasRelativeMode = false;
};