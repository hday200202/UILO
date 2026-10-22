#pragma once

#include "Input.hpp"
#include <functional>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace uilo {

/*
    Keybinds
    - Desc: Maps a semantic action name to one or more physical keys, polled
            once per frame rather than driven by events. Binding by physical
            key position instead of by character keeps a binding on the same
            key across keyboard layouts.
    - bindAction(name, key(s), callback, onPressOnly)
        onPressOnly = true  -> fires once on the press edge
        onPressOnly = false -> fires every frame the key is held
      Several keys bound to one action behave as "any of these", so
      { Key::Equals, Key::KpPlus } is a single action.
    - update() is driven by UILO::update(); an application only binds and
      unbinds. While a focused widget is consuming text input, UILO calls it
      with dispatch=false so typing never triggers application shortcuts.
    - The window comes from UILO's renderer; setWindow() is called by
      UILO::setRenderer(), so application code never handles it.
*/
class Keybinds {
public:
    using ActionCallback = std::function<void()>;

    struct Action {
        std::string      name;
        std::vector<Key> keys;
        ActionCallback   callback;
        bool             onPressOnly = false;
    };

    void setWindow(const valo::Window* window) { m_window = window; }

    void bindAction(
        const std::string& name,
        std::initializer_list<Key> keys,
        ActionCallback callback,
        bool onPressOnly = false
    ) {
        m_actions[name] = { name, keys, std::move(callback), onPressOnly };
        for (auto key : keys) m_prevState.try_emplace(key, false);
    }

    void bindAction(
        const std::string& name,
        Key key,
        ActionCallback callback,
        bool onPressOnly = false
    ) {
        bindAction(name, { key }, std::move(callback), onPressOnly);
    }

    void unbindAction(const std::string& name) { m_actions.erase(name); }
    void clear()                               { m_actions.clear(); m_prevState.clear(); }

    bool hasAction(const std::string& name) const {
        return m_actions.find(name) != m_actions.end();
    }

    /* True while any key bound to the action is held. */
    bool isActionDown(const std::string& name) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end() || !m_window) return false;
        for (auto key : it->second.keys)
            if (m_window->isKeyDown(key)) return true;
        return false;
    }

    const std::unordered_map<std::string, Action>& getActions() const { return m_actions; }

    /* dispatch=false advances the edge-detection state without firing any
       callback, so a key held while suppressed is not reported as a fresh. */
    void update(bool dispatch = true) {
        if (!m_window) return;

        if (dispatch) {
            for (auto& [name, action] : m_actions) {
                (void)name;
                bool isDown  = false;
                bool wasDown = false;

                for (auto key : action.keys) {
                    if (m_window->isKeyDown(key)) isDown = true;
                    auto prev = m_prevState.find(key);
                    if (prev != m_prevState.end() && prev->second) wasDown = true;
                }

                if (isDown && action.callback && (!action.onPressOnly || !wasDown))
                    action.callback();
            }
        }

        /* Snapshot after every action has been evaluated. */
        for (auto& [key, prev] : m_prevState) prev = m_window->isKeyDown(key);
    }

private:
    const valo::Window*                     m_window = nullptr;
    std::unordered_map<std::string, Action> m_actions;
    std::unordered_map<Key, bool>           m_prevState;
};

} // namespace uilo
