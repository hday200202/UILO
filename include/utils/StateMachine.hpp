#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace uilo {

// Transition
// Usage: Transition("Run", []() { return speed > 0.5f; })
// Transition to "Run" state if speed is > 0.5f.
struct Transition {
    std::string             targetState;
    std::function<bool()>   condition;

    Transition(const std::string& targetState, std::function<bool()> condition)
        : targetState(targetState), condition(condition) {}
};

// State
// Usage:  State(
//             "Idle",
//             []()        { },
//             [](float dt){ },
//             []()        { }
//         );
class State {
public:
    State(
        const std::string&          name,
        std::function<void()>       onEnter,
        std::function<void(float)>  onUpdate,
        std::function<void()>       onExit
    )
        : m_name(name), m_onEnter(onEnter), m_onUpdate(onUpdate), m_onExit(onExit) {}

    void onEnter()              { if (m_onEnter)    m_onEnter(); }
    void onUpdate(float dt)     { if (m_onUpdate)   m_onUpdate(dt); }
    void onExit()               { if (m_onExit)     m_onExit(); }

    const std::string&  getName() const         { return m_name; }
    void                setName(const std::string& name) { m_name = name; }

    void setOnEnter(std::function<void()> onEnter)          { m_onEnter = onEnter; }
    void setOnUpdate(std::function<void(float)> onUpdate)   { m_onUpdate = onUpdate; }
    void setOnExit(std::function<void()> onExit)            { m_onExit = onExit; }

    void addTransition(const Transition& transition)        { m_transitions.push_back(transition); }

    std::string checkTransitions() const {
        for (const auto& t : m_transitions)
            if (t.condition()) return t.targetState;
        return "";
    }

private:
    std::string                 m_name;
    std::function<void()>       m_onEnter   = nullptr;
    std::function<void(float)>  m_onUpdate  = nullptr;
    std::function<void()>       m_onExit    = nullptr;

    std::vector<Transition>     m_transitions;
};

// StateMachine
// Usage: StateMachine sm;
//        sm.addState(idleState);
//        sm.setState("Idle");
//        sm.update(deltaTime);    // call every frame
class StateMachine {
public:
    void addState(const State& state)   { m_states[state.getName()] = state; }

    void setState(const std::string& name) {
        if (m_currentState) m_currentState->onExit();
        m_currentState = &m_states.at(name);
        m_currentState->onEnter();
    }

    void update(float dt) {
        std::string next = m_currentState->checkTransitions();
        if (!next.empty()) { setState(next); return; }
        m_currentState->onUpdate(dt);
    }

    std::string getCurrentState() const {
        return m_currentState ? m_currentState->getName() : "";
    }

private:
    std::unordered_map<std::string, State>  m_states;
    State*                                  m_currentState = nullptr;
};

} // namespace uilo