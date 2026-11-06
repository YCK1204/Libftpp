#ifndef THREAD_HPP
#define THREAD_HPP

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include "thread_safe_iostream.hpp"


class Thread {
public:
  Thread() = delete;
  Thread(const Thread& other) = delete;
  Thread(const Thread&& other) = delete;
  Thread& operator = (const Thread& other) = delete;
  Thread(const std::string &name, std::function<void()> functToExecute);
  ~Thread();
  void start();
  void stop();
  bool isStop();

private:
  std::string m_threadName;
  std::thread m_thread;
  std::function<void()> m_startFunc;
  std::atomic<bool> m_stopRequest;
};

#endif