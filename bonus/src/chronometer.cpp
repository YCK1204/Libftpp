#include "chronometer.hpp"

Chronometer::Chronometer() {
}

void Chronometer::start() {
  reset();
  m_timer.start();
}

void Chronometer::stop() {
  m_timer.pause();
}

void Chronometer::resume() {
  m_timer.resume();
}

void Chronometer::reset() {
  m_laps.clear();
  m_timer.stop();
  m_elapsedTime = 0;
}

bool Chronometer::isRunning() const { return m_timer.isRunning(); }
double Chronometer::elapsed() const { return m_timer.elapsed(); }

double Chronometer::lap() {
  if (!m_timer.isPaused() && !m_timer.isRunning())
    return 0;
  double now = m_timer.elapsed();
  double segment = now - m_elapsedTime;
  if (m_timer.isPaused() && segment == 0.0)
    return 0;
  m_elapsedTime = now;
  m_laps.push_back(segment);
  return segment;
}


const std::vector<double> &Chronometer::laps() const {
  return m_laps;
}
