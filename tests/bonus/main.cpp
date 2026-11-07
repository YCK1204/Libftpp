#include "libftpp.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
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

#define CHECK_THROWS(expr, ExType)             \
  do {                                         \
    bool thrown = false;                       \
    try {                                      \
      expr;                                    \
    } catch (const ExType &) {                 \
      thrown = true;                           \
    }                                          \
    CHECK(thrown && #expr " throws " #ExType); \
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
  CHECK(c.elapsed() == frozen);                    // 이중 집계, 재시작 없음
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

// ---------------------------------------------------------------- RingBuffer

typedef RingBuffer<int> IntRing;

static std::string dump(const IntRing &ring) {
  std::string out;
  for (std::size_t i = 0; i < ring.size(); ++i)
    out += std::to_string(ring[i]) + ",";
  return out;
}

static void testRingBufferBasic() {
  std::cout << "[RingBuffer] basic" << std::endl;
  CHECK_THROWS(IntRing(0), std::invalid_argument);

  IntRing r(3);
  CHECK(r.empty() && !r.full());
  CHECK(r.capacity() == 3);
  CHECK(r.policy() == IntRing::OverflowPolicy::OVERWRITE);
  CHECK(r.push(1) && r.push(2) && r.push(3));
  CHECK(r.full() && r.size() == 3);
  CHECK(dump(r) == "1,2,3,");
  CHECK(r.front() == 1 && r.back() == 3);
  CHECK(r.at(1) == 2);
  CHECK_THROWS(r.at(3), std::out_of_range);
}

static void testRingBufferOverwrite() {
  std::cout << "[RingBuffer] overwrite" << std::endl;
  IntRing r(3);
  r.push(1);
  r.push(2);
  r.push(3);
  CHECK(r.push(4)); // 가장 오래된 1이 빠짐
  CHECK(dump(r) == "2,3,4,");
  for (int i = 5; i <= 10; ++i) // 인덱스가 여러 바퀴 돌아도 순서 유지
    r.push(i);
  CHECK(dump(r) == "8,9,10,");
  CHECK(r.front() == 8 && r.back() == 10);
  CHECK(r.size() == 3);
}

static void testRingBufferReject() {
  std::cout << "[RingBuffer] reject" << std::endl;
  IntRing r(2, IntRing::OverflowPolicy::REJECT);
  CHECK(r.push(1) && r.push(2));
  CHECK(!r.push(3));
  CHECK(dump(r) == "1,2,");
}

static void testRingBufferPop() {
  std::cout << "[RingBuffer] pop" << std::endl;
  IntRing r(3);
  for (int i = 1; i <= 5; ++i)
    r.push(i);
  CHECK(r.pop() == 3 && r.pop() == 4); // 오래된 것부터
  CHECK(dump(r) == "5,");
  r.push(6);
  r.push(7); // pop 후 다시 채워도 순서 유지
  CHECK(dump(r) == "5,6,7,");
  CHECK(r.pop() == 5 && r.pop() == 6 && r.pop() == 7);
  CHECK(r.empty());
  CHECK_THROWS(r.pop(), std::out_of_range);
  CHECK_THROWS(r.front(), std::out_of_range);
  CHECK_THROWS(r.back(), std::out_of_range);
}

static void testRingBufferClear() {
  std::cout << "[RingBuffer] clear" << std::endl;
  IntRing r(3);
  r.push(1);
  r.push(2);
  r.clear();
  CHECK(r.empty() && r.size() == 0);
  CHECK(r.capacity() == 3); // 칸 수는 그대로
  CHECK(r.push(7));
  CHECK(dump(r) == "7," && r.front() == 7 && r.back() == 7);
}

static void testRingBufferAccess() {
  std::cout << "[RingBuffer] const access / modify" << std::endl;
  IntRing r(2);
  r.push(1);
  r.push(2);
  r.front() = 10;
  r[1] = 20;
  const IntRing &cr = r;
  CHECK(cr.front() == 10 && cr.back() == 20);
  CHECK(cr[0] == 10 && cr.at(1) == 20);
  CHECK_THROWS(cr.at(2), std::out_of_range);
}

static void testRingBufferReleasesSlots() {
  std::cout << "[RingBuffer] releases popped values" << std::endl;
  std::shared_ptr<int> p = std::make_shared<int>(5);
  RingBuffer<std::shared_ptr<int>> r(2);
  r.push(p);
  CHECK(p.use_count() == 2);
  r.pop();
  CHECK(p.use_count() == 1); // pop한 칸이 참조를 놓음
  r.push(p);
  r.clear();
  CHECK(p.use_count() == 1);
  r.push(p);
  r.push(std::make_shared<int>(1));
  r.push(std::make_shared<int>(2)); // 덮어쓰기로 밀려나도 놓음
  CHECK(p.use_count() == 1);

  RingBuffer<std::string> s(2);
  s.push("a");
  s.push("b");
  s.push("c");
  CHECK(s.front() == "b" && s.back() == "c");
}

// ---------------------------------------------------------------- Logger

static bool endsWith(const std::string &s, const std::string &tail) {
  return s.size() >= tail.size() &&
         s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

// 테스트마다 같은 상태에서 시작. 콘솔은 꺼서 테스트 출력을 더럽히지 않음
static void resetLogger(std::size_t historySize) {
  Logger::setConsole(false);
  Logger::closeFile();
  Logger::setLevel(Logger::Level::Debug);
  Logger::setHistorySize(0);
  Logger::setHistorySize(historySize);
}

static void testLoggerFormat() {
  std::cout << "[Logger] line format" << std::endl;
  resetLogger(4);
  Logger::info("client {0} connected", 3);
  std::vector<std::string> h = Logger::history();
  CHECK(h.size() == 1);
  if (h.size() != 1)
    return;
  // [YYYY-MM-DD HH:MM:SS.mmm] [INFO ] client 3 connected
  const std::string &line = h[0];
  CHECK(line.size() > 25);
  CHECK(line[0] == '[' && line[5] == '-' && line[8] == '-');
  CHECK(line[11] == ' ' && line[14] == ':' && line[17] == ':');
  CHECK(line[20] == '.' && line[24] == ']');
  CHECK(endsWith(line, "] [INFO ] client 3 connected"));
  CHECK(line.find('\n') == std::string::npos); // history엔 개행 없음

  Logger::debug("d");
  Logger::warn("w");
  Logger::error("e");
  h = Logger::history();
  CHECK(h.size() == 4);
  if (h.size() == 4) {
    CHECK(endsWith(h[1], "[DEBUG] d"));
    CHECK(endsWith(h[2], "[WARN ] w"));
    CHECK(endsWith(h[3], "[ERROR] e"));
  }
}

static void testLoggerLevelFilter() {
  std::cout << "[Logger] level filter" << std::endl;
  resetLogger(10);
  Logger::setLevel(Logger::Level::Warn);
  CHECK(Logger::level() == Logger::Level::Warn);
  Logger::debug("no");
  Logger::info("no");
  Logger::warn("yes");
  Logger::error("yes");
  Logger::log(Logger::Level::Info, "no");
  Logger::log(Logger::Level::Error, "yes");
  std::vector<std::string> h = Logger::history();
  CHECK(h.size() == 3);
  for (std::size_t i = 0; i < h.size(); ++i)
    CHECK(endsWith(h[i], "yes"));
}

static void testLoggerOff() {
  std::cout << "[Logger] Off" << std::endl;
  resetLogger(10);
  Logger::setLevel(Logger::Level::Off);
  Logger::debug("x");
  Logger::error("x");
  Logger::log(Logger::Level::Error, "x");
  CHECK(Logger::history().empty()); // Off면 전부 꺼짐

  Logger::setLevel(Logger::Level::Debug);
  Logger::log(Logger::Level::Off, "x");
  CHECK(Logger::history().empty()); // Off는 메시지 레벨이 아님
}

static void testLoggerHistory() {
  std::cout << "[Logger] history" << std::endl;
  resetLogger(0);
  Logger::info("not kept");
  CHECK(Logger::history().empty()); // 기본 0: 보관 안 함

  Logger::setHistorySize(3);
  for (int i = 0; i < 5; ++i)
    Logger::info("m{0}", i);
  std::vector<std::string> h = Logger::history();
  CHECK(h.size() == 3);
  if (h.size() == 3)
    CHECK(endsWith(h[0], "m2") && endsWith(h[2], "m4")); // 오래된 순

  Logger::setHistorySize(5); // 키우면 기존 보관분 유지
  h = Logger::history();
  CHECK(h.size() == 3);

  Logger::setHistorySize(2); // 줄이면 최신 것만 남음
  h = Logger::history();
  CHECK(h.size() == 2);
  if (h.size() == 2)
    CHECK(endsWith(h[0], "m3") && endsWith(h[1], "m4"));

  h.clear(); // 복사본이라 원본에 영향 없음
  CHECK(Logger::history().size() == 2);

  Logger::setHistorySize(0); // 0이면 비우고 보관 끔
  CHECK(Logger::history().empty());
  Logger::info("not kept");
  CHECK(Logger::history().empty());
}

static std::vector<std::string> readLines(const std::string &path) {
  std::vector<std::string> lines;
  std::ifstream in(path.c_str());
  std::string line;
  while (std::getline(in, line))
    lines.push_back(line);
  return lines;
}

static void testLoggerFile() {
  std::cout << "[Logger] file" << std::endl;
  const std::string path = "test_logger_bonus.log";
  std::remove(path.c_str());
  resetLogger(0);

  Logger::openFile(path);
  Logger::info("first");
  Logger::closeFile();
  Logger::info("not in file"); // 닫은 뒤엔 기록 안 함

  Logger::openFile(path); // append: 기존 내용 유지
  Logger::warn("second");
  Logger::closeFile();

  std::vector<std::string> lines = readLines(path);
  CHECK(lines.size() == 2);
  if (lines.size() == 2) {
    CHECK(endsWith(lines[0], "[INFO ] first"));
    CHECK(endsWith(lines[1], "[WARN ] second"));
  }
  std::remove(path.c_str());

  CHECK_THROWS(Logger::openFile("/nonexistent_dir_libftpp/x.log"),
               std::runtime_error);
}

static void testLoggerThreads() {
  std::cout << "[Logger] concurrent logging" << std::endl;
  const int threads = 4;
  const int perThread = 200;
  resetLogger(threads * perThread);

  std::vector<std::thread> pool;
  for (int t = 0; t < threads; ++t)
    pool.push_back(std::thread([t]() {
      for (int i = 0; i < perThread; ++i)
        Logger::info("t{0} i{1}", t, i);
    }));
  for (std::size_t i = 0; i < pool.size(); ++i)
    pool[i].join();

  std::vector<std::string> h = Logger::history();
  CHECK(h.size() == static_cast<std::size_t>(threads * perThread));
  bool wellFormed = true;
  for (std::size_t i = 0; i < h.size(); ++i)
    if (h[i][0] != '[' || h[i].find("] [INFO ] t") == std::string::npos)
      wellFormed = false;
  CHECK(wellFormed); // 줄끼리 섞이지 않음
  resetLogger(0);
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

  testRingBufferBasic();
  testRingBufferOverwrite();
  testRingBufferReject();
  testRingBufferPop();
  testRingBufferClear();
  testRingBufferAccess();
  testRingBufferReleasesSlots();

  testLoggerFormat();
  testLoggerLevelFilter();
  testLoggerOff();
  testLoggerHistory();
  testLoggerFile();
  testLoggerThreads();

  std::cout << std::endl;
  if (g_failed) {
    std::cout << g_failed << " check(s) failed" << std::endl;
    return 1;
  }
  std::cout << "all bonus checks passed" << std::endl;
  return 0;
}
