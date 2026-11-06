#include "libftpp.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

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

  std::cout << std::endl;
  if (g_failed) {
    std::cout << g_failed << " check(s) failed" << std::endl;
    return 1;
  }
  std::cout << "all bonus checks passed" << std::endl;
  return 0;
}
