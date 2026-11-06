#include "libftpp.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static int g_failed = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (cond) {                                                                \
      std::cout << "  [OK]   " << #cond << std::endl;                          \
    } else {                                                                   \
      std::cout << "  [FAIL] " << #cond << " (" << __FILE__ << ":" << __LINE__ \
                << ")" << std::endl;                                           \
      ++g_failed;                                                              \
    }                                                                          \
  } while (0)

#define CHECK_THROWS(expr, ExType)                                             \
  do {                                                                         \
    bool thrown = false;                                                       \
    try {                                                                      \
      expr;                                                                    \
    } catch (const ExType &) {                                                 \
      thrown = true;                                                           \
    }                                                                          \
    CHECK(thrown && #expr " throws " #ExType);                                 \
  } while (0)

static void sleepMs(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// 실제 시간을 쓰므로 하한은 정확히, 상한은 스케줄링 지연을 감안해 넉넉히 검사
static const double SLACK = 0.1;

static bool around(double value, double expected) {
  return value >= expected && value < expected + SLACK;
}

// ---------------------------------------------------------------- StringFormat

struct Point {
  int x;
  int y;
};

static std::ostream &operator<<(std::ostream &os, const Point &p) {
  return os << "(" << p.x << ", " << p.y << ")";
}

static void testStringFormatBasic() {
  std::cout << "[StringFormat] basic" << std::endl;
  CHECK(StringFormat::format("{0} + {1} = {2}", 1, 2, 3) == "1 + 2 = 3");
  CHECK(StringFormat::format("{1} {0}", "world", "hello") == "hello world");
  CHECK(StringFormat::format("{0}{0}{0}", 'a') == "aaa");
  CHECK(StringFormat::format("pi={0} ok={1}", 3.5, true) == "pi=3.5 ok=1");
  CHECK(StringFormat::format("p={0}", Point{1, 2}) == "p=(1, 2)");
  CHECK(StringFormat::format("") == "");
  CHECK(StringFormat::format("plain text") == "plain text");
}

static void testStringFormatNoReinterpretation() {
  std::cout << "[StringFormat] substituted values are not reparsed" << std::endl;
  // 예전 구현은 여기서 무한 루프
  CHECK(StringFormat::format("{0}", "{0}") == "{0}");
  // 예전 구현은 "a=X b=X"
  CHECK(StringFormat::format("a={0} b={1}", "{1}", "X") == "a={1} b=X");
}

static void testStringFormatEdgeCases() {
  std::cout << "[StringFormat] edge cases" << std::endl;
  CHECK(StringFormat::format("{{0}} {0}", 7) == "{0} 7");
  CHECK(StringFormat::format("}} {{") == "} {");
  CHECK(StringFormat::format("{1}", "only") == "{1}");
  CHECK(StringFormat::format("no args {0}") == "no args {0}");
  CHECK(StringFormat::format("{}", 1) == "{}");
  CHECK(StringFormat::format("{x}", 1) == "{x}");
  CHECK(StringFormat::format("{0", 1) == "{0");
  CHECK(StringFormat::format("{99999999999999999999999}", 1) == "{99999999999999999999999}");
  CHECK(StringFormat::format("{00}", "z") == "z");
  CHECK(StringFormat::format("end {", 1) == "end {");
}

// ---------------------------------------------------------------- Timer

static void testTimerIdle() {
  std::cout << "[Timer] idle state" << std::endl;
  Timer t(1.0);
  CHECK(t.duration() == 1.0);
  CHECK(!t.isRunning());
  CHECK(!t.isPaused());
  CHECK(!t.isTimedOut());
  CHECK(t.elapsed() == 0.0);
  CHECK(t.remaining() == 1.0);

  Timer zero;
  CHECK(zero.duration() == 0.0);
  CHECK(!zero.isTimedOut()); // 시작 전에는 duration 0이어도 타임아웃 아님
}

static void testTimerRunAndTimeout() {
  std::cout << "[Timer] run and timeout" << std::endl;
  Timer t(0.1);
  t.start();
  CHECK(t.isRunning());
  CHECK(!t.isTimedOut());
  sleepMs(50);
  CHECK(!t.isTimedOut());
  CHECK(around(t.elapsed(), 0.05));
  CHECK(t.remaining() > 0.0 && t.remaining() <= 0.05);
  sleepMs(70);
  CHECK(t.isTimedOut());
  CHECK(t.remaining() == 0.0);
  CHECK(t.isRunning()); // 타임아웃돼도 자동으로 멈추지 않음

  Timer zero;
  zero.start();
  CHECK(zero.isTimedOut());
}

static void testTimerPauseResume() {
  std::cout << "[Timer] pause / resume" << std::endl;
  Timer t(1.0);
  t.start();
  sleepMs(50);
  t.pause();
  CHECK(t.isPaused());
  CHECK(!t.isRunning());
  double frozen = t.elapsed();
  CHECK(around(frozen, 0.05));
  sleepMs(100);
  CHECK(t.elapsed() == frozen); // 일시정지 동안 시간 안 흐름

  t.resume();
  CHECK(t.isRunning());
  sleepMs(50);
  CHECK(around(t.elapsed(), 0.1)); // 멈춘 100ms는 제외

  t.pause();
  double once = t.elapsed();
  sleepMs(20);
  t.pause(); // 두 번째 pause는 무시
  CHECK(t.elapsed() == once);
}

static void testTimerIgnoredCalls() {
  std::cout << "[Timer] calls in wrong state are ignored" << std::endl;
  Timer idle(1.0);
  idle.pause();
  CHECK(!idle.isPaused());
  idle.resume();
  CHECK(!idle.isRunning());

  Timer running(1.0);
  running.start();
  sleepMs(50);
  running.resume(); // Running 중 resume은 무시, 흐른 시간 유지
  CHECK(around(running.elapsed(), 0.05));
}

static void testTimerStopAndRestart() {
  std::cout << "[Timer] stop / restart" << std::endl;
  Timer t(0.05);
  t.start();
  sleepMs(70);
  CHECK(t.isTimedOut());

  t.start(); // 진행 중에도 0부터 다시 시작
  CHECK(!t.isTimedOut());
  CHECK(t.elapsed() < SLACK);

  sleepMs(20);
  t.stop();
  CHECK(!t.isRunning());
  CHECK(!t.isPaused());
  CHECK(!t.isTimedOut());
  CHECK(t.elapsed() == 0.0);

  t.pause(); // stop 후 pause는 무시
  CHECK(!t.isPaused());
}

static void testTimerDuration() {
  std::cout << "[Timer] duration" << std::endl;
  CHECK_THROWS(Timer(-1.0), std::invalid_argument);

  Timer t(2.0);
  CHECK_THROWS(t.setDuration(-0.5), std::invalid_argument);
  CHECK(t.duration() == 2.0); // 실패하면 값 유지

  t.start();
  sleepMs(50);
  t.setDuration(0.01); // 진행 중 변경: 흐른 시간은 유지, 목표만 바뀜
  CHECK(t.isTimedOut());
  CHECK(t.isRunning());
}

// ---------------------------------------------------------------- Chronometer

static double sum(const std::vector<double> &values) {
  double total = 0;
  for (std::size_t i = 0; i < values.size(); ++i)
    total += values[i];
  return total;
}

static void testChronometerIdle() {
  std::cout << "[Chronometer] idle state" << std::endl;
  Chronometer c;
  CHECK(!c.isRunning());
  CHECK(c.elapsed() == 0.0);
  CHECK(c.laps().empty());
  CHECK(c.lap() == 0.0); // 시작 전 lap은 0, 기록 안 함
  CHECK(c.laps().empty());
  c.stop();
  c.resume(); // 시작 전 stop/resume은 무시
  CHECK(!c.isRunning());
}

static void testChronometerRunning() {
  std::cout << "[Chronometer] elapsed while running" << std::endl;
  Chronometer c;
  c.start();
  CHECK(c.isRunning());
  sleepMs(50);
  CHECK(around(c.elapsed(), 0.05));
}

static void testChronometerStopResume() {
  std::cout << "[Chronometer] stop / resume" << std::endl;
  Chronometer c;
  c.start();
  sleepMs(50);
  c.stop();
  CHECK(!c.isRunning());
  double frozen = c.elapsed();
  sleepMs(100);
  CHECK(c.elapsed() == frozen); // 멈춘 동안 안 흐름
  c.resume();
  CHECK(c.isRunning());
  sleepMs(50);
  CHECK(around(c.elapsed(), 0.1)); // 멈춘 100ms 제외
}

static void testChronometerLaps() {
  std::cout << "[Chronometer] laps" << std::endl;
  Chronometer c;
  c.start();
  sleepMs(50);
  double first = c.lap();
  CHECK(around(first, 0.05));
  CHECK(c.isRunning()); // lap은 시계를 멈추지 않음
  sleepMs(30);
  double second = c.lap();
  CHECK(around(second, 0.03)); // 누적이 아니라 직전 lap부터의 구간
  CHECK(c.laps().size() == 2);
  CHECK(c.laps()[0] == first && c.laps()[1] == second);
  CHECK(around(c.elapsed(), 0.08)); // elapsed는 총 시간

  c.stop();
  sleepMs(50);
  c.resume();
  sleepMs(20);
  CHECK(around(c.lap(), 0.02)); // 멈춘 구간은 lap에서도 제외
  CHECK(c.elapsed() - sum(c.laps()) < SLACK);
}

static void testChronometerLapWhileStopped() {
  std::cout << "[Chronometer] lap while stopped" << std::endl;
  Chronometer c;
  c.start();
  sleepMs(30);
  c.lap();
  sleepMs(30);
  c.stop();
  double frozen = c.elapsed();

  double last = c.lap(); // 멈춘 시점까지를 마지막 구간으로 기록
  CHECK(around(last, 0.03));
  CHECK(!c.isRunning()); // lap 후에도 멈춘 상태 유지
  CHECK(c.laps().size() == 2);
  CHECK(c.lap() == 0.0); // 연속 lap은 빈 구간이라 기록 안 함
  CHECK(c.laps().size() == 2);

  sleepMs(50);
  CHECK(c.elapsed() == frozen); // 이중 집계, 재시작 없음
  CHECK(std::fabs(sum(c.laps()) - frozen) < 1e-9); // 부동소수 덧셈 오차만 허용
}

static void testChronometerRestartAndReset() {
  std::cout << "[Chronometer] restart / reset" << std::endl;
  Chronometer c;
  c.start();
  sleepMs(30);
  c.lap();
  c.start(); // 0부터 다시, 랩 기록도 초기화
  CHECK(c.isRunning());
  CHECK(c.laps().empty());
  CHECK(c.elapsed() < SLACK);
  sleepMs(20);
  CHECK(around(c.lap(), 0.02)); // 이전 lap 기준점이 남아있지 않음

  c.reset();
  CHECK(!c.isRunning());
  CHECK(c.elapsed() == 0.0);
  CHECK(c.laps().empty());
}

int main() {
  testStringFormatBasic();
  testStringFormatNoReinterpretation();
  testStringFormatEdgeCases();

  testTimerIdle();
  testTimerRunAndTimeout();
  testTimerPauseResume();
  testTimerIgnoredCalls();
  testTimerStopAndRestart();
  testTimerDuration();

  testChronometerIdle();
  testChronometerRunning();
  testChronometerStopResume();
  testChronometerLaps();
  testChronometerLapWhileStopped();
  testChronometerRestartAndReset();

  std::cout << std::endl;
  if (g_failed) {
    std::cout << g_failed << " check(s) failed" << std::endl;
    return 1;
  }
  std::cout << "all bonus checks passed" << std::endl;
  return 0;
}
