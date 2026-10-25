#ifndef WORKER_POOL_HPP
#define WORKER_POOL_HPP

#include "Define.hpp"
#include "IJobs.hpp"
#include "thread.hpp"
#include "thread_safe_queue.hpp"
#include <condition_variable>
#include <functional>
#include <memory>
#include <vector>

class WorkerPool {
public:
  WorkerPool(const uint8 &numOfThreads);
  WorkerPool(const WorkerPool &) = delete;
  WorkerPool(const WorkerPool &&) = delete;
  WorkerPool &operator=(const WorkerPool &) = delete;
  ~WorkerPool();

public:
  void addJob(const std::function<void()> &job);
  
private:
  void workerThread();

private:
  std::vector<std::unique_ptr<Thread>> m_threads;
  std::condition_variable m_cvForJob;
  std::mutex m_mutex;
  bool m_stopAll;
  ThreadSafeQueue<std::unique_ptr<IJobs>> m_jobs;
};


#endif