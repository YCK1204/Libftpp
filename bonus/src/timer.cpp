#include "timer.hpp"
#include "string_format.hpp"
#include <algorithm>
#include <stdexcept>

Timer::Timer() : m_duration(0) {
}

Timer::Timer(double seconds) {
  if (seconds < 0)
    throw std::invalid_argument("Timer: [" + StringFormat::toString(seconds) + "] is not valid argument");
  m_duration = seconds;
}

void Timer::setDuration(double seconds) {
  if (seconds < 0)
    throw std::invalid_argument("Timer: [" + StringFormat::toString(seconds) + "] is not valid argument");
  m_duration = seconds;
}

double Timer::duration() const { return m_duration; }

void Timer::start() {
  m_startTime = Clock::now();
  m_state = Timer::TimerState::RUNNING;
  m_elapsedTime = 0;
}

void Timer::stop() {
  m_elapsedTime = 0;
  m_state = TimerState::IDLE;
}

void Timer::pause() {
  if (m_state != TimerState::RUNNING)
    return;
  Clock::time_point pauseTime = Clock::now();
  auto elapsed = pauseTime - m_startTime;
  m_elapsedTime += std::chrono::duration<double>(elapsed).count();
  m_state = TimerState::PAUSE;
}

void Timer::resume() {
  if (m_state != TimerState::PAUSE)
    return;
  m_startTime = Clock::now();
  m_state = TimerState::RUNNING;
}

bool Timer::isRunning() const { return m_state == TimerState::RUNNING; }
bool Timer::isPaused() const { return m_state == TimerState::PAUSE; }
bool Timer::isTimedOut() const {
  if (m_state == TimerState::IDLE)
    return 0;
  return (m_duration <= elapsed());
}
double Timer::elapsed() const {
  double result = 0;
  switch (m_state) {
  case TimerState::RUNNING: {
    Clock::time_point endTime = Clock::now();
    auto elapsed = endTime - m_startTime;
    result = m_elapsedTime + std::chrono::duration<double>(elapsed).count();
  } break;
  case TimerState::PAUSE:
    result = m_elapsedTime;
    break;
  case TimerState::IDLE:
    break;
  }
  return result;
}

double Timer::remaining() const {
  return std::max(m_duration - elapsed(), 0.0);
}