#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace uilo {

/*
    Transition
    Usage: Transition("Run", []() { return speed > 0.5f; })
    Transition to "Run" state if speed is > 0.5f.
*/
struct Transition {
    std::string             targetState;
    std::function<bool()>   condition;

    Transition(const std::string& targetState, std::function<bool()> condition)
    : targetState(targetState), condition(condition) {}
};


/*
    State
    Usage:  State(
                "Idle",
                []()        { },
                [](float dt){ },
                []()        { }
            );
*/
class State {
public:
    State(
        const std::string&          name,
        std::function<void()>       onEnter,
        std::function<void(float)>  onUpdate,
        std::function<void()>       onExit
    ) : m_name(name), m_onEnter(onEnter), m_onUpdate(onUpdate), m_onExit(onExit) {}

    void OnEnter()              { if (m_onEnter)    m_onEnter(); }
    void OnUpdate(float dt)     { if (m_onUpdate)   m_onUpdate(dt); }
    void OnExit()               { if (m_onExit)     m_onExit(); }

    const std::string&  GetName() const         { return m_name; }
    void                SetName(const std::string& name) { m_name = name; }

    void SetOnEnter(std::function<void()> onEnter)          { m_onEnter = onEnter; }
    void SetOnUpdate(std::function<void(float)> onUpdate)   { m_onUpdate = onUpdate; }
    void SetOnExit(std::function<void()> onExit)            { m_onExit = onExit; }

    void AddTransition(const Transition& transition)        { m_transitions.push_back(transition); }

    std::string CheckTransitions() const {
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


/*
    StateMachine
    Usage: StateMachine sm;
           sm.AddState(idleState);
           sm.SetState("Idle");
           sm.Update(deltaTime);    // call every frame
*/
class StateMachine {
public:
    void AddState(const State& state)   { m_states[state.GetName()] = state; }

    void SetState(const std::string& name) {
        if (m_currentState) m_currentState->OnExit();
        m_currentState = &m_states.at(name);
        m_currentState->OnEnter();
    }

    void Update(float dt) {
        std::string next = m_currentState->CheckTransitions();
        if (!next.empty()) { SetState(next); return; }
        m_currentState->OnUpdate(dt);
    }

    std::string GetCurrentState() const {
        return m_currentState ? m_currentState->GetName() : "";
    }

private:
    std::unordered_map<std::string, State>  m_states;
    State*                                  m_currentState = nullptr;
};

}