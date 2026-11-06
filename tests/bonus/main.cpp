#include "libftpp.hpp"

#include <iostream>
#include <string>

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

int main() {
  testStringFormatBasic();
  testStringFormatNoReinterpretation();
  testStringFormatEdgeCases();

  std::cout << std::endl;
  if (g_failed) {
    std::cout << g_failed << " check(s) failed" << std::endl;
    return 1;
  }
  std::cout << "all bonus checks passed" << std::endl;
  return 0;
}
