#include "worker_pool.hpp"
#include "IJobs.hpp"
#include "thread.hpp"
#include <memory>
#include <mutex>
#include <string>
#include "thread_safe_iostream.hpp"

WorkerPool::WorkerPool(const uint8 &numOfThreads) : m_stopAll(false) {
  m_threads.reserve(numOfThreads);

  for (uint8 i = 0; i < numOfThreads; i++) {
    std::string threadName = "Thread " + std::to_string(i);
    std::function<void()> startFunc = [this]() { workerThread(); };
    m_threads.push_back(std::unique_ptr<Thread>(new Thread(threadName, startFunc)));
    m_threads.back()->start();
  }
}

WorkerPool::~WorkerPool() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stopAll = true;
  }
  m_cvForJob.notify_all();
  for (std::size_t i = 0; i < m_threads.size(); i++) {
    m_threads[i]->stop();
  }
}

void WorkerPool::addJob(const std::function<void()> &jobToExecute) {
  std::unique_ptr<Job> ptr(new Job(jobToExecute));
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_jobs.push_back(std::move(ptr));
  }
  m_cvForJob.notify_one();
}

void WorkerPool::workerThread() {
  while (true) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cvForJob.wait(lock, [this]() { return !this->m_jobs.empty() || m_stopAll; });
    if (m_stopAll && this->m_jobs.empty()) {
      return;
    }

    std::unique_ptr<IJobs> job = m_jobs.pop_front();
    lock.unlock();

    try {
      job->execute();
    } catch (const std::exception &e) {
      threadSafeCout << "job failed: " << e.what() << std::endl;
    } catch (...) {
      threadSafeCout << "job failed: unknown exception" << std::endl;
    }
  }
}
