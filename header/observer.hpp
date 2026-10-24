#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include <functional>
#include <map>
#include <vector>

template <typename TEvent>
class Observer {
private:
  std::map<TEvent, std::vector<std::function<void()>>> m_events;

public:
  void subscribe(const TEvent &event, const std::function<void()> &lambda);
  void unsubscribe(const TEvent &event);
  void notify(const TEvent &event);
};

template <typename TEvent>
inline void Observer<TEvent>::subscribe(const TEvent &event, const std::function<void()> &lambda) {
  m_events[event].push_back(lambda);
}

template <typename TEvent>
inline void Observer<TEvent>::unsubscribe(const TEvent &event) {
  m_events.erase(event);
}

template <typename TEvent>
inline void Observer<TEvent>::notify(const TEvent &event) {
  auto it = m_events.find(event);

  if (m_events.end() == it)
    return;
  auto callbacks = it->second;

  for (auto itt = callbacks.begin(); itt != callbacks.end(); itt++) {
    (*itt)();
  }
}

#endif