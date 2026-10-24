#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <cstddef>
#include <functional>
#include <map>
#include <stdexcept>
#include <vector>

template <typename TState>
class StateMachine {
public:
  void addState(const TState &state);
  void addTransition(const TState &startState, const TState &finalState, const std::function<void()> &lambda);
  void addAction(const TState &state, const std::function<void()> &lambda);
  void transitionTo(const TState &state);
  void update();

public:
  void removeAction(const TState &state);
  void removeTransition(const TState &startState, const TState &finalState);

public:
  StateMachine();
  StateMachine(const TState &state);

private:
  std::map<TState, std::map<TState, std::vector<std::function<void()>>>> m_transitions;
  std::map<TState, std::vector<std::function<void()>>> m_actions;
  TState m_curState;
  bool m_hasState = false;
};

template <typename TState>
inline void StateMachine<TState>::addState(const TState &state) {
  m_transitions[state];
  if (m_hasState == false) {
    m_curState = state;
    m_hasState = true;
  }
}

template <typename TState>
inline void StateMachine<TState>::addTransition(const TState &startState, const TState &finalState, const std::function<void()> &lambda) {
  m_transitions[startState][finalState].push_back(lambda);
}

template <typename TState>
inline void StateMachine<TState>::addAction(const TState &state, const std::function<void()> &lambda) {
  m_actions[state].push_back(lambda);
}

template <typename TState>
inline void StateMachine<TState>::transitionTo(const TState &state) {
  if (!m_hasState)
    throw std::runtime_error("StateMachine: current state not set");

  auto it = m_transitions[m_curState].find(state);

  if (it != m_transitions[m_curState].end()) {
    std::vector<std::function<void()>> callbacks = it->second;
    for (std::size_t i = 0; i < callbacks.size(); i++) {
      (callbacks[i])();
    }
    m_curState = state;
  } else {
    throw std::runtime_error("StateMachine: transition not defined");
  }
}

template <typename TState>
inline void StateMachine<TState>::update() {
  if (!m_hasState)
    throw std::runtime_error("StateMachine: current state not set");

  std::vector<std::function<void()>> actions = m_actions[m_curState];

  if (actions.empty())
    throw std::runtime_error("StateMachine: no action for current state");
  for (auto action : actions)
    (action)();
}

template <typename TState>
inline void StateMachine<TState>::removeAction(const TState &state) {
  m_actions.erase(state);
}

template <typename TState>
inline void StateMachine<TState>::removeTransition(const TState &startState, const TState &finalState) {
  auto it = m_transitions.find(startState);

  if (it != m_transitions.end()) {
    it->second.erase(finalState);
  }
}

template <typename TState>
inline StateMachine<TState>::StateMachine() {}
template <typename TState>
inline StateMachine<TState>::StateMachine(const TState &state) { addState(state); }

#endif