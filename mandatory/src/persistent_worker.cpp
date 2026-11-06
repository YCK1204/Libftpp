#include "persistent_worker.hpp"
#include "thread_safe_iostream.hpp"
#include <exception>
#include <mutex>
#include <stdexcept>

PersistentWorker::PersistentWorker(const std::string &name) : m_stop(false), m_thread(name, [this]() { this->run(); }) {
  m_thread.start();
}

void PersistentWorker::addTask(const std::string &name, const std::function<void()> &jobToExecute) {
  Lock lock(m_mutex);

  if (m_tasks.find(name) != m_tasks.end())
    throw std::invalid_argument("PersistentWorker: [" + name + "] is already running");

  m_tasks[name] = jobToExecute;
  m_cv.notify_one();
}

void PersistentWorker::removeTask(const std::string &name) {
  Lock lock(m_mutex);

  auto it = m_tasks.find(name);
  if (it == m_tasks.end())
    throw std::invalid_argument("PersistentWorker: [" + name + "] is not set, but tried remove");
  m_tasks.erase(name);
}

PersistentWorker::~PersistentWorker() {
  {
    Lock lock(m_mutex);
    m_stop = true;
  }
  m_cv.notify_one();
}

void PersistentWorker::run() {
  while (true) {
    std::map<std::string, std::function<void()>> tasks;
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock, [this]() { return m_stop || !this->m_tasks.empty(); });
      if (m_stop)
        break;
      tasks = m_tasks;
    }

    for (auto it = tasks.begin(); it != tasks.end(); it++) {
      try {
        (it->second)();
      } catch (std::exception &e) {
        threadSafeCout << e.what() << std::endl;
      } catch (...) {
        threadSafeCout << "PersistentWorker: unknown exception" << std::endl;
      }
    }
  }
}
