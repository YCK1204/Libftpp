#ifndef STRING_FORMAT_HPP
#define STRING_FORMAT_HPP

#include <sstream>
#include <string>
#include <vector>

class StringFormat {
public:
  StringFormat() = delete;

  template <typename T>
  static std::string toString(const T &value);

  template <typename... TArgs>
  static std::string format(const std::string &src, const TArgs &...args);

private:
  static std::string substitute(const std::string &src, const std::vector<std::string> &values);
};

template <typename T>
inline std::string StringFormat::toString(const T &value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

template <typename... TArgs>
inline std::string StringFormat::format(const std::string &src, const TArgs &...args) {
  std::vector<std::string> values = {toString(args)...};
  return substitute(src, values);
}

#endif
