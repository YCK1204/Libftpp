#ifndef PERSISTENT_WORKER_HPP
#define PERSISTENT_WORKER_HPP

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include "thread.hpp"

class PersistentWorker {
public:
  PersistentWorker(const std::string &name);
  void addTask(const std::string &name, const std::function<void()> &jobToExecute);
  void removeTask(const std::string &name);
  ~PersistentWorker();

private:
  void run();

private:
  using Lock = std::lock_guard<std::mutex>;
  std::mutex m_mutex;
  std::map<std::string, std::function<void()>> m_tasks;
  bool m_stop;
  std::condition_variable m_cv;
  Thread m_thread;
};

#endif