#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace uilo {

/*
    Mousebinds
    - Desc: The mouse counterpart to Keybinds. Button actions are polled per
            frame by name; motion actions receive relative deltas.
    - bindButton(name, button(s), callback, onPressOnly)
        Buttons are SDL_BUTTON_LEFT / _RIGHT / _MIDDLE / _X1 / _X2. Several
        buttons bound to one action behave as "any of these".
        Widget clicks are unrelated to this -- Modifier::setOnLeftClick is the
        hit-tested, per-element path. Use Mousebinds for window-wide gestures.
    - bindMotion(name, callback)
        Fires with (dx, dy) in logical pixels while relative mode is on, which
        is the mode a camera or orbit control wants: the cursor is hidden and
        locked, so only deltas are meaningful. Nothing is dispatched while
        relative mode is off, and the frame the lock is taken is skipped so the
        jump to the warp position is not reported as motion.
    - The SDL_Window comes from UILO's renderer; setWindow() is called by
      UILO::setRenderer(), so application code never handles it.
*/
class Mousebinds {
public:
    using MotionAction   = std::function<void(float dx, float dy)>;
    using ButtonCallback = std::function<void()>;

    struct MotionActionItem {
        std::string  name;
        MotionAction callback;
    };

    struct ButtonActionItem {
        std::string           name;
        std::vector<uint32_t> buttonMasks;   /* pre-computed SDL_BUTTON_MASK per button */
        ButtonCallback        callback;
        bool                  onPressOnly = false;
    };

    Mousebinds() = default;

    void setWindow(SDL_Window* window) { m_window = window; }
    SDL_Window* getWindow() const      { return m_window; }

    /* Hides and locks the cursor, switching motion reporting to pure deltas. */
    void setRelativeMode(bool enabled) {
        if (!m_window) {
            std::fprintf(stderr, "[UILO] Mousebinds::setRelativeMode: no window yet "
                                 "(call UILO::setRenderer first)\n");
            return;
        }
        if (!SDL_SetWindowRelativeMouseMode(m_window, enabled)) {
            std::fprintf(stderr, "[UILO] Mousebinds::setRelativeMode(%d) failed: %s\n",
                         (int)enabled, SDL_GetError());
        }
    }

    bool getRelativeMode() const {
        return m_window ? SDL_GetWindowRelativeMouseMode(m_window) : false;
    }

    void bindMotion(const std::string& name, MotionAction callback) {
        m_motionActions[name] = { name, std::move(callback) };
    }

    void unbindMotion(const std::string& name) { m_motionActions.erase(name); }

    void bindButton(
        const std::string& name,
        std::initializer_list<uint8_t> buttons,
        ButtonCallback callback,
        bool onPressOnly = false
    ) {
        std::vector<uint32_t> masks;
        masks.reserve(buttons.size());
        for (uint8_t button : buttons) {
            const uint32_t mask = SDL_BUTTON_MASK(button);
            masks.push_back(mask);
            m_prevButtonState.try_emplace(mask, false);
        }
        m_buttonActions[name] = { name, std::move(masks), std::move(callback), onPressOnly };
    }

    void bindButton(
        const std::string& name,
        uint8_t button,
        ButtonCallback callback,
        bool onPressOnly = false
    ) {
        bindButton(name, { button }, std::move(callback), onPressOnly);
    }

    void unbindButton(const std::string& name) { m_buttonActions.erase(name); }

    void clear() {
        m_buttonActions.clear();
        m_motionActions.clear();
        m_prevButtonState.clear();
    }

    bool isButtonActionDown(const std::string& name) const {
        auto it = m_buttonActions.find(name);
        if (it == m_buttonActions.end()) return false;
        const uint32_t buttons = SDL_GetMouseState(nullptr, nullptr);
        for (uint32_t mask : it->second.buttonMasks)
            if ((buttons & mask) != 0) return true;
        return false;
    }

    /* dispatch=false advances edge-detection state without firing callbacks,
       matching Keybinds::update(). */
    void update(bool dispatch = true) {
        const uint32_t currentButtons = SDL_GetMouseState(nullptr, nullptr);

        if (dispatch) {
            for (auto& [name, action] : m_buttonActions) {
                (void)name;
                bool isDown  = false;
                bool wasDown = false;

                for (uint32_t mask : action.buttonMasks) {
                    if ((currentButtons & mask) != 0) isDown = true;
                    auto prev = m_prevButtonState.find(mask);
                    if (prev != m_prevButtonState.end() && prev->second) wasDown = true;
                }

                if (isDown && action.callback && (!action.onPressOnly || !wasDown))
                    action.callback();
            }
        }

        /* Snapshot once every action has been evaluated, for the same reason
           as Keybinds: actions sharing a button must all see the same prior. */
        for (auto& [mask, prev] : m_prevButtonState)
            prev = ((currentButtons & mask) != 0);

        /* Motion: relative mode only, and never on the frame the lock is
           taken. */
        const bool relativeMode = getRelativeMode();
        const bool justLocked    = relativeMode && !m_wasRelativeMode;
        m_wasRelativeMode        = relativeMode;

        float dx = 0.f;
        float dy = 0.f;
        SDL_GetRelativeMouseState(&dx, &dy);

        if (!dispatch || !relativeMode || justLocked) return;
        if (dx == 0.f && dy == 0.f) return;
        if (m_motionActions.empty()) return;

        /* Deltas arrive in backing-store pixels; divide out the display scale
           so handlers work in the same logical units as the rest of the API. */
        const float scale = m_window ? SDL_GetWindowDisplayScale(m_window) : 1.f;
        if (scale > 0.f) { dx /= scale; dy /= scale; }

        for (const auto& [name, action] : m_motionActions) {
            (void)name;
            if (action.callback) action.callback(dx, dy);
        }
    }

private:
    SDL_Window* m_window = nullptr;

    std::unordered_map<std::string, MotionActionItem> m_motionActions;
    std::unordered_map<std::string, ButtonActionItem> m_buttonActions;
    std::unordered_map<uint32_t, bool>               m_prevButtonState;
    bool m_wasRelativeMode = false;
};

} // namespace uilo
