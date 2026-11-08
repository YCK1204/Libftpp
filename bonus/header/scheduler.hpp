#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "timer.hpp"
#include <cstddef>
#include <functional>
#include <map>
class Scheduler {
public:
  using TaskId = std::size_t;
  TaskId after(double seconds, const std::function<void()> &task);
  TaskId every(double interval, const std::function<void()> &task);
  bool cancel(TaskId id);
  bool has(TaskId id) const;
  void update();
  std::size_t size() const;
  void clear();

private:
  struct Task {
    bool every;
    std::function<void()> task;
    Task(double time, bool isEvery, const std::function<void()> &task);
    Timer timer;
  };
  TaskId m_nextId = 0;
  std::map<TaskId, Task> m_tasks;
};

#endif
