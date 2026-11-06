#ifndef THREAD_SAFE_IOSTREAM_HPP
#define THREAD_SAFE_IOSTREAM_HPP

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

extern thread_local class ThreadSafeIOStream threadSafeCout;

class ThreadSafeIOStream {
public:
  template <typename T>
  ThreadSafeIOStream &operator<<(const T &value);
  ThreadSafeIOStream &operator<<(std::ostream &(*manip)(std::ostream &));
  template <typename T>
  ThreadSafeIOStream &operator>>(T &value);

  template <typename T>
  void prompt(const std::string &question, T &dest);
  void setPrefix(const std::string &prefix);
  ~ThreadSafeIOStream();

private:
  using Lock = std::lock_guard<std::mutex>;
  std::string m_prefix;
  std::ostringstream m_buffer;
  static std::mutex m_mutex;
  void tryFlush();
};

template <typename T>
inline void ThreadSafeIOStream::prompt(const std::string &question, T &dest) {
    Lock lock(m_mutex);

    std::cout << m_prefix << question << std::flush;
    std::cin >> dest;
}

template <typename T>
inline ThreadSafeIOStream &ThreadSafeIOStream::operator>>(T &value) {
  Lock lock(m_mutex);

  std::cin >> value;
  return *this;
}

template <typename T>
inline ThreadSafeIOStream &ThreadSafeIOStream::operator<<(const T &value) {
  m_buffer << value;

  tryFlush();
  return *this;
}

#endif