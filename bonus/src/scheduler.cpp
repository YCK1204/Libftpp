#include "scheduler.hpp"
#include <stdexcept>
#include <vector>

Scheduler::TaskId Scheduler::after(double seconds, const std::function<void()> &task) {
  if (seconds < 0)
    throw std::invalid_argument("Scheduler: seconds is not valid");

  TaskId id = ++m_nextId;
  m_tasks.emplace(id, Task(seconds, false, task));
  return id;
}

Scheduler::TaskId Scheduler::every(double interval, const std::function<void()> &task) {
  if (interval <= 0)
    throw std::invalid_argument("Scheduler: interval is not valid");
  TaskId id = ++m_nextId;
  m_tasks.emplace(id, Task(interval, true, task));
  return id;
}

bool Scheduler::cancel(TaskId id) {
  auto it = m_tasks.find(id);
  if (m_tasks.end() != it) {
    m_tasks.erase(it);
    return true;
  }
  return false;
}

bool Scheduler::has(TaskId id) const {
  auto it = m_tasks.find(id);
  return (m_tasks.end() != it);
}

void Scheduler::update() {
  std::vector<TaskId> due;
  for (auto &it : m_tasks)
    if (it.second.timer.isTimedOut())
      due.push_back(it.first);

  for (TaskId id : due) {
    auto it = m_tasks.find(id);
    if (it == m_tasks.end())
      continue;
    std::function<void()> fn = it->second.task;
    if (it->second.every)
      it->second.timer.start();
    else
      m_tasks.erase(it);
    fn();
  }
}

std::size_t Scheduler::size() const { return m_tasks.size(); }

void Scheduler::clear() {
  m_tasks.clear();
}

Scheduler::Task::Task(double time, bool isEvery, const std::function<void()> &task) : every(isEvery), task(task), timer(time) {
  timer.start();
}
