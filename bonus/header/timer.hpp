#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
class Timer {
public:
  Timer();
  explicit Timer(double seconds);
  void setDuration(double seconds);
  double duration() const;
  void start();
  void stop();
  void pause();
  void resume();
  bool isRunning() const;
  bool isPaused() const;
  bool isTimedOut() const;
  double elapsed() const;
  double remaining() const;

private:
  enum class TimerState { IDLE, RUNNING, PAUSE };
  using Clock = std::chrono::steady_clock;
  double m_duration;
  double m_elapsedTime = 0;
  Clock::time_point m_startTime = Clock::time_point();
  TimerState m_state = Timer::TimerState::IDLE;
};

#endif