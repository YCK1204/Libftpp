#ifndef CHRONOMETER_HPP
#define CHRONOMETER_HPP

#include "timer.hpp"
#include <vector>
class Chronometer {
public:
  Chronometer();
  void start();
  void stop();
  void resume();
  void reset();
  bool isRunning() const;
  double elapsed() const;
  double lap();
  const std::vector<double> &laps() const;
private:
  std::vector<double> m_laps;
  Timer m_timer;
  double m_elapsedTime = 0;
};

#endif