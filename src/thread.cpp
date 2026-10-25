#include "thread.hpp"
#include "thread_safe_iostream.hpp"
#include <exception>
#include <ostream>

Thread::Thread(const std::string &name, std::function<void()> functToExecute) : m_threadName(name), m_startFunc(functToExecute), m_stopRequest(false) {
}

Thread::~Thread() {
  stop();
}

void Thread::start() {
  if (m_thread.joinable())
    return;

  m_stopRequest.store(false);
  m_thread = std::thread([this]() {
    try {
      threadSafeCout.setPrefix("[" + m_threadName + "] ");
      (m_startFunc)();
    } catch (std::exception &e) {
      threadSafeCout << e.what() << std::endl;
    } catch (...) {
      threadSafeCout << "Thread: unknown exception" << std::endl;
    }
  });
}

void Thread::stop() {
  m_stopRequest.store(true);

  if (m_thread.joinable())
    m_thread.join();
}

bool Thread::isStop() {
  return m_stopRequest.load();
}
