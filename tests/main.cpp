#include "data_structures.hpp"
#include "design_patterns.hpp"
#include "thread_safe_iostream.hpp"
#include "thread_safe_queue.hpp"
#include "thread.hpp"
#include "worker_pool.hpp"
#include "persistent_worker.hpp"
#include "message.hpp"
#include "network.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
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

// ---------------------------------------------------------------- DataBuffer

static void testDataBufferPrimitives() {
  std::cout << "[DataBuffer] primitives" << std::endl;
  DataBuffer db;
  db << 42 << 3.5f << 'c' << true << static_cast<int64>(-1);

  int i = 0;
  float f = 0;
  char c = 0;
  bool b = false;
  int64 l = 0;
  db >> i >> f >> c >> b >> l;
  CHECK(i == 42);
  CHECK(f == 3.5f);
  CHECK(c == 'c');
  CHECK(b == true);
  CHECK(l == -1);
}

static void testDataBufferString() {
  std::cout << "[DataBuffer] string" << std::endl;
  DataBuffer db;
  std::string longStr(100, 'x');
  std::string empty;
  db << longStr << empty << "literal";

  std::string r1, r2, r3;
  db >> r1 >> r2 >> r3;
  CHECK(r1 == longStr);
  CHECK(r2.empty());
  CHECK(r3 == "literal");
}

static void testDataBufferVector() {
  std::cout << "[DataBuffer] vector" << std::endl;
  DataBuffer db;
  std::vector<int> vi;
  vi.push_back(1);
  vi.push_back(2);
  vi.push_back(3);
  std::vector<std::string> vs;
  vs.push_back("a");
  vs.push_back("hello");
  std::vector<std::vector<int> > nested(2, vi);
  db << vi << vs << nested << std::vector<int>();

  std::vector<int> ri(5, 9); // 기존 내용이 남지 않아야 함
  std::vector<std::string> rs;
  std::vector<std::vector<int> > rn;
  std::vector<int> re(1, 7);
  db >> ri >> rs >> rn >> re;
  CHECK(ri == vi);
  CHECK(rs == vs);
  CHECK(rn == nested);
  CHECK(re.empty());
}

static void testDataBufferMap() {
  std::cout << "[DataBuffer] map / unordered_map" << std::endl;
  DataBuffer db;
  std::map<std::string, std::vector<int> > m;
  m["a"] = std::vector<int>(2, 1);
  m["bb"] = std::vector<int>();
  std::unordered_map<int, std::string> u;
  u[1] = "one";
  u[2] = "two";
  db << m << u;

  std::map<std::string, std::vector<int> > rm;
  rm["stale"] = std::vector<int>(1, 9); // 기존 내용이 남지 않아야 함
  std::unordered_map<int, std::string> ru;
  ru[100] = "stale";
  db >> rm >> ru;
  CHECK(rm == m);
  CHECK(ru == u);
}

static void testDataBufferSet() {
  std::cout << "[DataBuffer] set" << std::endl;
  DataBuffer db;
  std::set<std::string> s;
  s.insert("a");
  s.insert("hello");
  std::set<int> empty;
  db << s << empty;

  std::set<std::string> rs;
  rs.insert("stale"); // 기존 내용이 남지 않아야 함
  std::set<int> re;
  re.insert(7);
  db >> rs >> re;
  CHECK(rs == s);
  CHECK(re.empty());
}

static void testDataBufferErrors() {
  std::cout << "[DataBuffer] errors" << std::endl;
  {
    DataBuffer db;
    db << 1;
    int a = 0, b = 0;
    db >> a;
    CHECK_THROWS(db >> b, std::out_of_range);
  }
  {
    DataBuffer db; // count는 5인데 실제 원소는 1개
    db << static_cast<std::size_t>(5) << 1;
    std::vector<int> v(1, 42);
    CHECK_THROWS(db >> v, std::out_of_range);
    CHECK(v.size() == 1 && v[0] == 42); // 실패해도 원래 값 유지
  }
  {
    DataBuffer db; // 문자열 길이가 실제 데이터보다 김
    db << static_cast<std::size_t>(1000) << 'x';
    std::string s;
    CHECK_THROWS(db >> s, std::out_of_range);
  }
  {
    DataBuffer db;
    const char *nullStr = nullptr;
    CHECK_THROWS(db << nullStr, std::invalid_argument);
  }
}

static void testDataBufferCopy() {
  std::cout << "[DataBuffer] copy / clear" << std::endl;
  DataBuffer a;
  a << 7;
  const DataBuffer constCopy(a);
  DataBuffer assigned;
  assigned = a;
  CHECK(constCopy.getBufferSize() == a.getBufferSize());
  int v = 0;
  assigned >> v;
  CHECK(v == 7);
  a.clear();
  CHECK(a.getBufferSize() == 0);
}

// ---------------------------------------------------------------------- Pool

struct Tracked {
  static int alive;
  std::string name;
  int hp;
  Tracked(const std::string &n, int h) : name(n), hp(h) { ++alive; }
  ~Tracked() { --alive; }
};
int Tracked::alive = 0;

static void testPool() {
  std::cout << "[Pool] acquire / release" << std::endl;
  Pool<Tracked> pool;
  pool.resize(2);
  {
    Pool<Tracked>::Object a = pool.acquire("Goblin", 30);
    Pool<Tracked>::Object b = pool.acquire("Orc", 50);
    CHECK(a->name == "Goblin" && a->hp == 30);
    CHECK(b->name == "Orc" && b->hp == 50);
    CHECK(Tracked::alive == 2);
    a->hp -= 5;
    CHECK(a->hp == 25);
  }
  CHECK(Tracked::alive == 0); // Object 소멸 시 TType 소멸자 호출

  std::cout << "[Pool] reuse / grow" << std::endl;
  {
    Pool<Tracked>::Object x = pool.acquire("X", 1);
    Pool<Tracked>::Object y = pool.acquire("Y", 2);
    Pool<Tracked>::Object z = pool.acquire("Z", 3); // 빈 칸이 없어 자동 확장
    CHECK(x.operator->() != y.operator->());
    CHECK(y.operator->() != z.operator->());
    CHECK(x->name == "X" && y->name == "Y" && z->name == "Z");
    CHECK(Tracked::alive == 3);
  }
  CHECK(Tracked::alive == 0);
}

// ------------------------------------------------------------------- Memento

class SaveablePlayer : public Memento {
  friend class Memento;

public:
  int hp;
  std::string name;
  std::vector<int> items;

  SaveablePlayer() : hp(100), name("hero") {}

private:
  void _saveToSnapshot(Memento::Snapshot &snapshot) const {
    snapshot << hp << name << items;
  }
  void _loadFromSnapshot(Memento::Snapshot &snapshot) {
    snapshot >> hp >> name >> items;
  }
};

static void testMemento() {
  std::cout << "[Memento] save / load" << std::endl;
  SaveablePlayer p;
  p.items.push_back(1);
  p.items.push_back(2);
  Memento::Snapshot snap = p.save();

  p.hp = 1;
  p.name = "dead";
  p.items.clear();
  p.load(snap);
  CHECK(p.hp == 100 && p.name == "hero" && p.items.size() == 2);

  p.hp = 5;
  p.load(snap); // 같은 스냅샷으로 다시 복원 가능해야 함
  CHECK(p.hp == 100);

  Memento *base = new SaveablePlayer();
  delete base; // 가상 소멸자
  CHECK(true && "delete via Memento* compiles and runs");
}

// ------------------------------------------------------------------ Observer

enum class TestEvent { A, B, C };

static void testObserver() {
  std::cout << "[Observer] subscribe / notify" << std::endl;
  Observer<TestEvent> obs;
  int a = 0;
  obs.subscribe(TestEvent::A, [&a]() { a += 1; });
  obs.subscribe(TestEvent::A, [&a]() { a += 10; });
  obs.notify(TestEvent::A);
  CHECK(a == 11); // 같은 이벤트의 구독자 모두 실행

  obs.notify(TestEvent::C); // 구독자 없는 이벤트
  CHECK(a == 11);

  std::cout << "[Observer] subscribe inside callback" << std::endl;
  int b = 0;
  obs.subscribe(TestEvent::B, [&]() {
    b += 1;
    obs.subscribe(TestEvent::B, [&b]() { b += 100; });
  });
  obs.notify(TestEvent::B);
  CHECK(b == 1); // 새 구독은 다음 notify부터
  obs.notify(TestEvent::B);
  CHECK(b == 102);

  std::cout << "[Observer] unsubscribe" << std::endl;
  obs.unsubscribe(TestEvent::A);
  a = 0;
  obs.notify(TestEvent::A);
  CHECK(a == 0);
}

// ----------------------------------------------------------------- Singleton

class TestConfig : public Singleton<TestConfig> {
  friend class Singleton<TestConfig>;

public:
  std::string name;
  int value;

private:
  TestConfig(const std::string &n, int v) : name(n), value(v) {}
};

static void testSingleton() {
  std::cout << "[Singleton] instantiate / instance" << std::endl;
  CHECK(TestConfig::instance() == nullptr); // instantiate 전

  TestConfig::instantiate("cfg", 3);
  CHECK(TestConfig::instance() != nullptr);
  CHECK(TestConfig::instance()->name == "cfg" &&
        TestConfig::instance()->value == 3);
  CHECK(TestConfig::instance() == TestConfig::instance()); // 항상 같은 인스턴스

  CHECK_THROWS(TestConfig::instantiate("again", 4), std::runtime_error);
  CHECK(TestConfig::instance()->name == "cfg"); // 예외 후에도 기존 인스턴스 유지
}

// -------------------------------------------------------------- StateMachine

enum class TestState { Idle, Run, Dead };

static void testStateMachine() {
  std::cout << "[StateMachine] initial state / update" << std::endl;
  {
    StateMachine<TestState> sm;
    CHECK_THROWS(sm.update(), std::runtime_error); // 상태가 하나도 없음
    CHECK_THROWS(sm.transitionTo(TestState::Run), std::runtime_error);

    std::string log;
    sm.addState(TestState::Idle); // 첫 addState가 초기 상태
    sm.addState(TestState::Run);
    sm.addAction(TestState::Idle, [&log]() { log += "idle;"; });
    sm.addAction(TestState::Run, [&log]() { log += "run;"; });
    sm.addTransition(TestState::Idle, TestState::Run,
                     [&log]() { log += "go;"; });

    sm.update();
    sm.transitionTo(TestState::Run);
    sm.update();
    CHECK(log == "idle;go;run;");

    std::cout << "[StateMachine] undefined transition / action" << std::endl;
    CHECK_THROWS(sm.transitionTo(TestState::Idle), std::runtime_error);
    log.clear();
    sm.update(); // 실패한 전이 후에도 Run 유지
    CHECK(log == "run;");

    sm.addState(TestState::Dead);
    sm.addTransition(TestState::Run, TestState::Dead, []() {});
    sm.transitionTo(TestState::Dead);
    CHECK_THROWS(sm.update(), std::runtime_error); // Dead에 액션 없음
  }

  std::cout << "[StateMachine] constructor with initial state" << std::endl;
  {
    StateMachine<TestState> sm(TestState::Run);
    sm.addState(TestState::Idle); // 이미 초기 상태가 있으므로 바뀌지 않음
    int runCount = 0;
    sm.addAction(TestState::Run, [&runCount]() { ++runCount; });
    sm.update();
    CHECK(runCount == 1);
  }

  std::cout << "[StateMachine] throwing transition keeps state" << std::endl;
  {
    StateMachine<TestState> sm;
    sm.addState(TestState::Idle);
    sm.addState(TestState::Run);
    sm.addTransition(TestState::Idle, TestState::Run,
                     []() { throw std::logic_error("boom"); });
    int idleCount = 0;
    sm.addAction(TestState::Idle, [&idleCount]() { ++idleCount; });
    CHECK_THROWS(sm.transitionTo(TestState::Run), std::logic_error);
    sm.update();
    CHECK(idleCount == 1);
  }

  std::cout << "[StateMachine] removeAction / removeTransition" << std::endl;
  {
    StateMachine<TestState> sm;
    sm.addState(TestState::Idle);
    sm.addState(TestState::Run);
    sm.addAction(TestState::Idle, []() {});
    sm.addTransition(TestState::Idle, TestState::Run, []() {});
    sm.addTransition(TestState::Run, TestState::Idle, []() {});

    sm.removeAction(TestState::Idle);
    CHECK_THROWS(sm.update(), std::runtime_error);

    sm.removeTransition(TestState::Idle, TestState::Run);
    CHECK_THROWS(sm.transitionTo(TestState::Run), std::runtime_error);

    sm.removeAction(TestState::Dead);                       // 없는 액션
    sm.removeTransition(TestState::Dead, TestState::Idle);  // 없는 전이
    CHECK(true && "removing missing entries does not throw");
  }
}

// -------------------------------------------------------- ThreadSafeIOStream

// std::cout 출력을 문자열로 가로채는 RAII 헬퍼
class CoutCapture {
public:
  CoutCapture() : m_old(std::cout.rdbuf(m_captured.rdbuf())) {}
  ~CoutCapture() { std::cout.rdbuf(m_old); }
  std::string str() const { return m_captured.str(); }

private:
  std::ostringstream m_captured;
  std::streambuf *m_old;
};

static std::vector<std::string> splitLines(const std::string &s) {
  std::vector<std::string> lines;
  std::istringstream in(s);
  std::string line;
  while (std::getline(in, line))
    lines.push_back(line);
  return lines;
}

static void testThreadSafeIOStream() {
  std::cout << "[ThreadSafeIOStream] prefix / newline handling" << std::endl;
  {
    std::string out;
    {
      CoutCapture capture;
      threadSafeCout.setPrefix("[main] ");
      threadSafeCout << "first" << std::endl;
      threadSafeCout << "a\nb\n";
      threadSafeCout << "num " << 42 << std::flush << " same line" << std::endl;
      threadSafeCout << "partial";
      out = capture.str();
    }
    CHECK(out == "[main] first\n[main] a\n[main] b\n[main] num 42 same line\n");
    {
      CoutCapture capture;
      threadSafeCout << " done" << std::endl; // 개행 전 내용과 합쳐져야 함
      out = capture.str();
    }
    CHECK(out == "[main] partial done\n");
  }

  std::cout << "[ThreadSafeIOStream] multi-thread lines do not interleave"
            << std::endl;
  {
    const int threadCount = 4;
    const int linesPerThread = 100;
    std::string out;
    {
      CoutCapture capture;
      std::vector<std::thread> threads;
      for (int t = 0; t < threadCount; ++t) {
        threads.push_back(std::thread([t]() {
          threadSafeCout.setPrefix("[T" + std::to_string(t) + "] ");
          for (int i = 0; i < linesPerThread; ++i)
            threadSafeCout << "line " << i << std::endl;
        }));
      }
      for (std::size_t i = 0; i < threads.size(); ++i)
        threads[i].join();
      out = capture.str();
    }

    std::vector<std::string> lines = splitLines(out);
    CHECK(lines.size() == static_cast<std::size_t>(threadCount * linesPerThread));

    std::vector<int> nextExpected(threadCount, 0);
    bool allWellFormed = true;
    bool inOrderPerThread = true;
    for (std::size_t i = 0; i < lines.size(); ++i) {
      int t = -1, n = -1;
      std::istringstream in(lines[i]);
      char open, letter, close;
      std::string word;
      if (!(in >> open >> letter >> t >> close >> word >> n) || open != '[' ||
          letter != 'T' || close != ']' || word != "line" || t < 0 ||
          t >= threadCount) {
        allWellFormed = false;
        continue;
      }
      if (n != nextExpected[t])
        inOrderPerThread = false;
      nextExpected[t] = n + 1;
    }
    CHECK(allWellFormed);    // 모든 줄이 "[Tn] line i" 형태 (섞임 없음)
    CHECK(inOrderPerThread); // 스레드별 출력 순서 유지
  }
}

// ----------------------------------------------------------- ThreadSafeQueue

struct NoDefaultCtor {
  explicit NoDefaultCtor(int v) : value(v) {}
  int value;
};

static void testThreadSafeQueue() {
  std::cout << "[ThreadSafeQueue] push / pop order" << std::endl;
  {
    ThreadSafeQueue<std::string> q;
    q.push_back("b");
    q.push_front("a");
    q.push_back("c");
    CHECK(q.pop_front() == "a");
    CHECK(q.pop_back() == "c");
    CHECK(q.pop_front() == "b");
    CHECK_THROWS(q.pop_back(), std::runtime_error);
    CHECK_THROWS(q.pop_front(), std::runtime_error);
  }
  {
    ThreadSafeQueue<NoDefaultCtor> q; // 기본 생성자 없는 타입
    q.push_back(NoDefaultCtor(7));
    CHECK(q.pop_front().value == 7);
  }

  std::cout << "[ThreadSafeQueue] concurrent push / pop" << std::endl;
  {
    const int producerCount = 4;
    const int itemsPerProducer = 5000;
    ThreadSafeQueue<int> q;

    std::vector<std::thread> producers;
    for (int p = 0; p < producerCount; ++p) {
      producers.push_back(std::thread([&q, p]() {
        for (int i = 0; i < itemsPerProducer; ++i)
          q.push_back(p * itemsPerProducer + i);
      }));
    }
    for (std::size_t i = 0; i < producers.size(); ++i)
      producers[i].join();

    std::vector<std::vector<int> > popped(producerCount);
    std::vector<std::thread> consumers;
    for (int c = 0; c < producerCount; ++c) {
      consumers.push_back(std::thread([&q, &popped, c]() {
        for (;;) {
          try {
            popped[c].push_back(c % 2 ? q.pop_back() : q.pop_front());
          } catch (const std::runtime_error &) {
            break; // 큐가 비면 종료
          }
        }
      }));
    }
    for (std::size_t i = 0; i < consumers.size(); ++i)
      consumers[i].join();

    std::vector<bool> seen(producerCount * itemsPerProducer, false);
    bool noDuplicate = true;
    std::size_t total = 0;
    for (std::size_t c = 0; c < popped.size(); ++c) {
      for (std::size_t i = 0; i < popped[c].size(); ++i) {
        int v = popped[c][i];
        if (seen[v])
          noDuplicate = false;
        seen[v] = true;
        ++total;
      }
    }
    CHECK(total == static_cast<std::size_t>(producerCount * itemsPerProducer));
    CHECK(noDuplicate); // 같은 원소를 두 스레드가 꺼내지 않음
  }
}

// -------------------------------------------------------------------- Thread

static void testThread() {
  std::cout << "[Thread] start / stop / prefix" << std::endl;
  {
    std::string out;
    {
      CoutCapture capture;
      threadSafeCout.setPrefix("[main] ");
      Thread worker("Worker", []() { threadSafeCout << "hello" << std::endl; });
      worker.start();
      worker.stop();
      threadSafeCout << "after" << std::endl;
      out = capture.str();
    }
    CHECK(out == "[Worker] hello\n[main] after\n"); // main의 prefix는 그대로
  }

  std::cout << "[Thread] not started until start()" << std::endl;
  {
    std::atomic<int> runs(0);
    Thread t("Counter", [&runs]() { ++runs; });
    CHECK(runs == 0);
    t.start();
    t.stop();
    CHECK(runs == 1);
    t.start(); // stop 후 재시작
    t.stop();
    CHECK(runs == 2);
    t.stop(); // 중복 stop 안전
    CHECK(runs == 2);
  }

  std::cout << "[Thread] exceptions inside thread" << std::endl;
  {
    std::string out;
    {
      CoutCapture capture;
      Thread a("Thrower", []() { throw std::runtime_error("boom"); });
      Thread b("Int", []() { throw 42; });
      a.start();
      a.stop();
      b.start();
      b.stop();
      out = capture.str();
    }
    CHECK(out.find("[Thrower] ") != std::string::npos &&
          out.find("boom") != std::string::npos);
    CHECK(out.find("[Int] ") != std::string::npos);
  }

  std::cout << "[Thread] destructor joins running thread" << std::endl;
  {
    std::atomic<bool> finished(false);
    {
      Thread t("Scoped", [&finished]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        finished = true;
      });
      t.start();
    } // stop() 없이 소멸 → 소멸자가 join
    CHECK(finished);
  }
}

// ---------------------------------------------------------------- WorkerPool

static void testWorkerPool() {
  std::cout << "[WorkerPool] all jobs run before destruction" << std::endl;
  {
    std::atomic<int> count(0);
    {
      WorkerPool pool(4);
      for (int i = 0; i < 1000; ++i)
        pool.addJob([&count]() { ++count; });
    } // 소멸자가 남은 작업을 모두 처리한 뒤 종료
    CHECK(count == 1000);
  }

  std::cout << "[WorkerPool] jobs run in parallel" << std::endl;
  {
    std::set<std::thread::id> workerIds;
    std::mutex idsMutex;
    {
      WorkerPool pool(4);
      for (int i = 0; i < 8; ++i) {
        pool.addJob([&workerIds, &idsMutex]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(30));
          std::lock_guard<std::mutex> lock(idsMutex);
          workerIds.insert(std::this_thread::get_id());
        });
      }
    }
    CHECK(workerIds.size() > 1); // 여러 워커가 실제로 작업을 나눠 실행
  }

  std::cout << "[WorkerPool] addJob inside a job" << std::endl;
  {
    std::atomic<int> count(0);
    {
      WorkerPool pool(2);
      pool.addJob([&pool, &count]() {
        ++count;
        pool.addJob([&count]() { ++count; }); // 데드락 없어야 함
      });
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    CHECK(count == 2);
  }

  std::cout << "[WorkerPool] throwing job does not kill the worker" << std::endl;
  {
    std::atomic<int> count(0);
    std::string out;
    {
      CoutCapture capture;
      {
        WorkerPool pool(1); // 워커 하나뿐 → 죽으면 다음 작업이 실행 안 됨
        pool.addJob([]() { throw std::runtime_error("boom"); });
        pool.addJob([&count]() { ++count; });
      }
      out = capture.str();
    }
    CHECK(count == 1);
    CHECK(out.find("boom") != std::string::npos);
  }
}

// ---------------------------------------------------------- PersistentWorker

static void testPersistentWorker() {
  std::cout << "[PersistentWorker] tasks run repeatedly" << std::endl;
  {
    std::atomic<int> a(0), b(0);
    {
      PersistentWorker worker("PW");
      worker.addTask("a", [&a]() { ++a; });
      worker.addTask("b", [&b]() { ++b; });
      std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    CHECK(a > 1 && b > 1); // 한 번이 아니라 계속 반복
  }

  std::cout << "[PersistentWorker] removeTask stops the task" << std::endl;
  {
    std::atomic<int> a(0);
    PersistentWorker worker("PW");
    worker.addTask("a", [&a]() { ++a; });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    worker.removeTask("a");
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 진행 중인 바퀴가 끝날 시간
    int afterRemove = a;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    CHECK(a == afterRemove); // 지운 뒤로는 더 실행되지 않음

    worker.addTask("a", []() {}); // 지운 이름은 다시 등록 가능
    CHECK_THROWS(worker.addTask("a", []() {}), std::invalid_argument);
    CHECK_THROWS(worker.removeTask("missing"), std::invalid_argument);
  }

  std::cout << "[PersistentWorker] removeTask inside a task" << std::endl;
  {
    std::atomic<int> runs(0);
    {
      PersistentWorker worker("Self");
      worker.addTask("once", [&worker, &runs]() {
        ++runs;
        worker.removeTask("once"); // 데드락 없어야 함
      });
      std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    CHECK(runs == 1);
  }

  std::cout << "[PersistentWorker] throwing task does not stop others"
            << std::endl;
  {
    std::atomic<int> good(0);
    {
      CoutCapture capture; // 예외 메시지 출력 숨기기
      PersistentWorker worker("Ex");
      worker.addTask("bad", []() { throw std::runtime_error("bad task"); });
      worker.addTask("good", [&good]() { ++good; });
      std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    CHECK(good > 1);
  }

  std::cout << "[PersistentWorker] idle worker destructs without hang"
            << std::endl;
  {
    { PersistentWorker worker("Idle"); } // 작업 없이 바로 소멸
    CHECK(true && "idle worker destroyed");
  }
}

// ------------------------------------------------------------------- Message

enum TestPacket { PKT_CHAT = 300, PKT_MOVE = 7, PKT_PING = 9 };

static void appendBytes(std::vector<uint8> &dst, const std::vector<uint8> &src) {
  dst.insert(dst.end(), src.begin(), src.end());
}

static void testMessage() {
  std::cout << "[Message] type / << / >>" << std::endl;
  {
    Message m(PKT_CHAT);
    m << 42 << std::string("hello");
    CHECK(m.type() == PKT_CHAT);
    const Message &cm = m; // 액션은 const Message&를 받음
    int v = 0;
    std::string s;
    cm >> v >> s;
    CHECK(v == 42 && s == "hello");
  }

  std::cout << "[Message] serialize / deserialize round trip" << std::endl;
  {
    Message m(PKT_CHAT);
    m << 42 << std::string("hello world");
    std::vector<uint8> bytes = m.serialize();
    CHECK(bytes.size() == Message::SizeOfHeader + sizeof(int) +
                              sizeof(std::size_t) + 11);

    Message out(0);
    CHECK(Message::deserialize(bytes, out) == bytes.size());
    int v = 0;
    std::string s;
    out >> v >> s;
    CHECK(out.type() == PKT_CHAT && v == 42 && s == "hello world");
  }

  std::cout << "[Message] multiple frames in one buffer" << std::endl;
  {
    Message a(PKT_CHAT), b(PKT_MOVE), ping(PKT_PING); // ping: 빈 body
    a << std::string("hi");
    b << 1.5f << 2.5f;
    std::vector<uint8> stream;
    appendBytes(stream, a.serialize());
    appendBytes(stream, b.serialize());
    appendBytes(stream, ping.serialize());

    std::vector<int> types;
    std::size_t offset = 0, used;
    Message out(0);
    while ((used = Message::deserialize(stream, offset, out)) > 0) {
      types.push_back(out.type());
      offset += used;
    }
    CHECK(types.size() == 3 && types[0] == PKT_CHAT && types[1] == PKT_MOVE &&
          types[2] == PKT_PING);
    CHECK(offset == stream.size());
  }

  std::cout << "[Message] partial frames" << std::endl;
  {
    Message m(PKT_CHAT);
    m << std::string("partial frame test");
    std::vector<uint8> full = m.serialize();
    bool allIncomplete = true;
    for (std::size_t cut = 0; cut < full.size(); ++cut) {
      std::vector<uint8> part(full.begin(), full.begin() + cut);
      Message out(0);
      if (Message::deserialize(part, out) != 0)
        allIncomplete = false;
    }
    CHECK(allIncomplete); // 잘린 모든 경우 "덜 옴"
    Message out(0);
    CHECK(Message::deserialize(full, full.size() + 1, out) == 0); // offset 범위 밖
  }
}

// ---------------------------------------------------------------- Network

enum NetPacket { NET_TEXT = 1, NET_NUMBER = 2, NET_REPLY = 3, NET_UNKNOWN = 99 };

// pred가 참이 될 때까지 server/client의 update를 반복 (최대 2초)
template <typename Pred>
static bool pump(Server &server, std::vector<Client *> clients, Pred pred) {
  for (int i = 0; i < 2000; ++i) {
    server.update();
    for (std::size_t c = 0; c < clients.size(); ++c)
      clients[c]->update();
    if (pred())
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return false;
}

static int rawConnect(int port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ::connect(fd, (sockaddr *)&addr, sizeof(addr));
  return fd;
}

static void testNetworkBasic() {
  const int port = 46101;
  Server server;
  server.start(port);

  std::map<int, long long> idOf; // payload 값 -> 서버가 부여한 clientID
  std::vector<std::string> texts;
  server.defineAction(NET_NUMBER, [&](long long &id, const Message &msg) {
    int v;
    msg >> v;
    idOf[v] = id;
  });
  server.defineAction(NET_TEXT, [&](long long &, const Message &msg) {
    std::string s;
    msg >> s;
    texts.push_back(s);
  });

  Client a, b, c;
  std::vector<int> gotA, gotB, gotC;
  a.defineAction(NET_REPLY, [&](const Message &m) { int v; m >> v; gotA.push_back(v); });
  b.defineAction(NET_REPLY, [&](const Message &m) { int v; m >> v; gotB.push_back(v); });
  c.defineAction(NET_REPLY, [&](const Message &m) { int v; m >> v; gotC.push_back(v); });
  a.connect("127.0.0.1", port);
  b.connect("127.0.0.1", port);
  c.connect("127.0.0.1", port);
  std::vector<Client *> all;
  all.push_back(&a);
  all.push_back(&b);
  all.push_back(&c);

  Message ma(NET_NUMBER), mb(NET_NUMBER), mc(NET_NUMBER);
  ma << 1;
  mb << 2;
  mc << 3;
  a.send(ma);
  b.send(mb);
  c.send(mc);
  CHECK(pump(server, all, [&] { return idOf.size() == 3; }));
  CHECK(idOf[1] != idOf[2] && idOf[2] != idOf[3] && idOf[1] != idOf[3]); // 서로 다른 ID

  // sendTo: 지정한 클라이언트만 받음
  Message r1(NET_REPLY);
  r1 << 100;
  server.sendTo(r1, idOf[2]);
  CHECK(pump(server, all, [&] { return gotB.size() == 1; }));
  CHECK(gotA.empty() && gotB[0] == 100 && gotC.empty());

  // sendToArray
  Message r2(NET_REPLY);
  r2 << 200;
  std::vector<long long> ids;
  ids.push_back(idOf[1]);
  ids.push_back(idOf[3]);
  ids.push_back(999999); // 없는 ID는 무시
  server.sendToArray(r2, ids);
  CHECK(pump(server, all, [&] { return gotA.size() == 1 && gotC.size() == 1; }));
  CHECK(gotA[0] == 200 && gotC[0] == 200 && gotB.size() == 1);

  // sendToAll
  Message r3(NET_REPLY);
  r3 << 300;
  server.sendToAll(r3);
  CHECK(pump(server, all, [&] { return gotA.size() == 2 && gotB.size() == 2 && gotC.size() == 2; }));
  CHECK(gotA[1] == 300 && gotB[1] == 300 && gotC[1] == 300);

  // 한 번에 여러 메시지 + 4096바이트보다 큰 메시지 + 액션 없는 타입
  std::string big(100000, 'x');
  Message t1(NET_TEXT), t2(NET_TEXT), unknown(NET_UNKNOWN);
  t1 << std::string("hello");
  t2 << big;
  unknown << 1;
  a.send(t1);
  a.send(unknown);
  a.send(t2);
  CHECK(pump(server, all, [&] { return texts.size() == 2; }));
  CHECK(texts.size() == 2 && texts[0] == "hello" && texts[1] == big);
}

static void testNetworkFragmented() {
  const int port = 46102;
  Server server;
  server.start(port);
  std::vector<int> got;
  server.defineAction(NET_NUMBER, [&](long long &, const Message &msg) {
    int v;
    msg >> v;
    got.push_back(v);
  });

  int fd = rawConnect(port);
  Message m1(NET_NUMBER), m2(NET_NUMBER);
  m1 << 11;
  m2 << 22;
  std::vector<uint8> bytes = m1.serialize();
  std::vector<uint8> second = m2.serialize();
  bytes.insert(bytes.end(), second.begin(), second.end());

  std::vector<Client *> none;
  // 한 바이트씩 보내도 메시지 경계가 유지되어야 함
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    ::send(fd, &bytes[i], 1, MSG_NOSIGNAL);
    server.update();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (i + 1 < bytes.size() / 2)
      CHECK(got.empty()); // 첫 메시지가 다 오기 전엔 처리되지 않음
  }
  CHECK(pump(server, none, [&] { return got.size() == 2; }));
  CHECK(got.size() == 2 && got[0] == 11 && got[1] == 22);
  ::close(fd);
}

static void testNetworkDisconnect() {
  const int port = 46103;
  Client client;
  int got = 0;
  client.defineAction(NET_REPLY, [&](const Message &) { ++got; });
  {
    Server server;
    server.start(port);
    long long clientId = -1;
    server.defineAction(NET_NUMBER, [&](long long &id, const Message &) { clientId = id; });

    // 같은 포트에 두 번째 서버는 실패
    Server dup;
    CHECK_THROWS(dup.start(port), std::runtime_error);
    // start 두 번 호출도 실패
    CHECK_THROWS(server.start(port + 10), std::runtime_error);

    {
      Client temp;
      temp.connect("127.0.0.1", port);
      Message m(NET_NUMBER);
      m << 1;
      temp.send(m);
      std::vector<Client *> none;
      CHECK(pump(server, none, [&] { return clientId != -1; }));
    } // temp 소멸 → 연결 끊김
    for (int i = 0; i < 50; ++i) {
      server.update();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Message r(NET_REPLY);
    server.sendTo(r, clientId); // 끊긴 클라이언트에 보내도 죽지 않음
    server.sendToAll(r);
    CHECK(true && "send to disconnected client survives");

    client.connect("127.0.0.1", port);
    std::vector<Client *> one(1, &client);
    server.sendToAll(r); // 아직 accept 전이라 못 받을 수 있음
    pump(server, one, [&] { return false; });
    server.sendToAll(r);
    CHECK(pump(server, one, [&] { return got >= 1; }));
  } // 서버 소멸

  // 서버가 사라지면 update에서 끊김을 감지하고, 이후 send는 예외
  for (int i = 0; i < 100; ++i) {
    client.update();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  CHECK_THROWS(client.send(Message(NET_NUMBER)), std::runtime_error);

  // 잘못된 주소 / 열려 있지 않은 포트
  Client bad;
  CHECK_THROWS(bad.connect("not-an-ip", port), std::invalid_argument);
  CHECK_THROWS(bad.connect("127.0.0.1", 46199), std::runtime_error);
}

int main(void) {
  testDataBufferPrimitives();
  testDataBufferString();
  testDataBufferVector();
  testDataBufferMap();
  testDataBufferSet();
  testDataBufferErrors();
  testDataBufferCopy();
  testPool();
  testMemento();
  testObserver();
  testSingleton();
  testStateMachine();
  testThreadSafeIOStream();
  testThreadSafeQueue();
  testThread();
  testWorkerPool();
  testPersistentWorker();
  testMessage();
  testNetworkBasic();
  testNetworkFragmented();
  testNetworkDisconnect();

  std::cout << std::endl;
  if (g_failed == 0) {
    std::cout << "ALL TESTS PASSED" << std::endl;
    return 0;
  }
  std::cout << g_failed << " TEST(S) FAILED" << std::endl;
  return 1;
}
