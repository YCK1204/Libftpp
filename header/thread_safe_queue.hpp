#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <deque>
#include <mutex>
#include <stdexcept>
#include <utility>
template <typename TType>
class ThreadSafeQueue {
public:
  void push_back(const TType &newElement);
  void push_front(const TType &newElement);
  void push_back(TType &&newElement);
  void push_front(TType &&newElement);
  TType pop_back();
  TType pop_front();
  bool empty();

private:
  using Lock = std::lock_guard<std::mutex>;
  std::deque<TType> m_deque;
  std::mutex m_mutex;
};

template <typename TType>
inline void ThreadSafeQueue<TType>::push_back(const TType &newElement) {
  Lock lock(m_mutex);
  m_deque.push_back(newElement);
}

template <typename TType>
inline void ThreadSafeQueue<TType>::push_front(const TType &newElement) {
  Lock lock(m_mutex);
  m_deque.push_front(newElement);
}
template <typename TType>
inline void ThreadSafeQueue<TType>::push_back(TType &&newElement) {
  Lock lock(m_mutex);
  m_deque.push_back(std::move(newElement));
}
template <typename TType>
inline void ThreadSafeQueue<TType>::push_front(TType &&newElement) {
  Lock lock(m_mutex);
  m_deque.push_front(std::move(newElement));
}

template <typename TType>
inline TType ThreadSafeQueue<TType>::pop_back() {
  Lock lock(m_mutex);

  if (m_deque.empty())
    throw std::runtime_error("ThreadSafeQueue: queue is empty but you tried pop");
  TType ret = std::move(m_deque.back());
  m_deque.pop_back();
  return ret;
}

template <typename TType>
inline TType ThreadSafeQueue<TType>::pop_front() {
  Lock lock(m_mutex);

  if (m_deque.empty())
    throw std::runtime_error("ThreadSafeQueue: queue is empty but you tried pop");
  TType ret = std::move(m_deque.front());
  m_deque.pop_front();
  return ret;
}
template <typename TType>
inline bool ThreadSafeQueue<TType>::empty() {
  Lock lock(m_mutex);

  return m_deque.empty();
}

#endif