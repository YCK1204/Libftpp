#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "define.hpp"
#include "ring_buffer.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "string_format.hpp"

class Logger {
public:
  enum class Level {
    Debug,
    Info,
    Warn,
    Error,
    Off
  };
  Logger() = delete;
  static void setLevel(Level level);
  static Level level();
  template <typename... TArgs>
  static void debug(const std::string &fmt, const TArgs &...args);
  template <typename... TArgs>
  static void info(const std::string &fmt, const TArgs &...args);
  template <typename... TArgs>
  static void warn(const std::string &fmt, const TArgs &...args);
  template <typename... TArgs>
  static void error(const std::string &fmt, const TArgs &...args);

  static void log(Level level, const std::string &message);
  static void setConsole(bool enabled);
  static void openFile(const std::string &path);
  static void closeFile();
  static void setHistorySize(std::size_t size);
  static std::vector<std::string> history();

private:
  using Lock = std::lock_guard<std::mutex>;
  using Clock = std::chrono::system_clock;
  static std::atomic<Level> s_level;
  static bool s_showLogOnConsole;
  static std::ofstream s_file;
  static UniquePtr<RingBuffer<std::string>> s_histories;
  static std::mutex s_mutex;
  static std::unordered_map<Level, std::string> s_levelStr;
  static std::vector<std::string> historyUnlocked();
};

//  StringFormat::format(fmt, args...) 후 log(Level::Debug, ...)
template <typename... TArgs>
inline void Logger::debug(const std::string &fmt, const TArgs &...args) {
  if (s_level.load() > Level::Debug)
    return;
  log(Level::Debug, StringFormat::format(fmt, args...));
}
template <typename... TArgs>
inline void Logger::error(const std::string &fmt, const TArgs &...args) {
  if (s_level.load() > Level::Error)
    return;
  log(Level::Error, StringFormat::format(fmt, args...));
}
template <typename... TArgs>
inline void Logger::warn(const std::string &fmt, const TArgs &...args) {
  if (s_level.load() > Level::Warn)
    return;
  log(Level::Warn, StringFormat::format(fmt, args...));
}

template <typename... TArgs>
inline void Logger::info(const std::string &fmt, const TArgs &...args) {
  if (s_level.load() > Level::Info)
    return;
  log(Level::Info, StringFormat::format(fmt, args...));
}

#endif