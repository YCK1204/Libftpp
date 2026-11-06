#include "string_format.hpp"

std::string StringFormat::substitute(const std::string &src, const std::vector<std::string> &values) {
  std::string result;
  std::size_t i = 0;
  const std::size_t len = src.size();

  result.reserve(len);
  while (i < len) {
    char c = src[i];

    if ((c == '{' || c == '}') && i + 1 < len && src[i + 1] == c) {
      result += c;
      i += 2;
      continue;
    }
    if (c == '{') {
      std::size_t j = i + 1;
      std::size_t index = 0;
      bool inRange = true;

      while (j < len && src[j] >= '0' && src[j] <= '9') {
        if (inRange) {
          index = index * 10 + (src[j] - '0');
          inRange = index < values.size();
        }
        ++j;
      }
      if (j > i + 1 && j < len && src[j] == '}' && inRange) {
        result += values[index];
        i = j + 1;
        continue;
      }
    }
    result += c;
    ++i;
  }
  return result;
}
