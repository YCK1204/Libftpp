#include "logger.hpp"
#include "define.hpp"
#include "ring_buffer.hpp"
#include "thread_safe_iostream.hpp"
#include <atomic>
#include <chrono>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <ctime>

std::atomic<Logger::Level> Logger::s_level(Logger::Level::Debug);
bool Logger::s_showLogOnConsole = true;
UniquePtr<RingBuffer<std::string>> Logger::s_histories = nullptr;
std::unordered_map<Logger::Level, std::string> Logger::s_levelStr = {
    {Logger::Level::Debug, "DEBUG"},
    {Logger::Level::Info, "INFO "},
    {Logger::Level::Warn, "WARN "},
    {Logger::Level::Error, "ERROR"},
};
std::mutex Logger::s_mutex;
std::ofstream Logger::s_file;

void Logger::setLevel(Level level) {
  Lock lock(s_mutex);
  s_level.store(level);
}

Logger::Level Logger::level() { return s_level.load(); }

void Logger::log(Level level, const std::string &message) {
  if (s_level.load() > level || level == Logger::Level::Off)
    return;

  auto now = std::chrono::system_clock::now();

  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  auto timer = std::chrono::system_clock::to_time_t(now);
  Lock lock(s_mutex);
  std::tm bt = *std::localtime(&timer);

  std::ostringstream oss;
  ///[2026-11-06 14:03:01.512][INFO ] client 3 connected

  oss << std::put_time(&bt, "[%Y-%m-%d %H:%M:%S"); // HH:MM:SS
  oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << "]";
  oss << ' ' << '[' << s_levelStr[level] << "] " << message;

  std::string str = oss.str();
  if (s_showLogOnConsole)
    threadSafeCout << str << '\n';
  if (s_histories != nullptr)
    s_histories->push(str);
  s_file << str << '\n';
}

void Logger::setConsole(bool enabled) {
  Lock lock(s_mutex);
  s_showLogOnConsole = enabled;
}

void Logger::openFile(const std::string &path) {
  Lock lock(s_mutex);
  if (s_file.is_open())
    s_file.close();
  s_file.open(path.c_str(), std::ios::app);
  if (!s_file.is_open() || s_file.fail())
    throw std::runtime_error("Logger: can not open [" + path + "]");
}

void Logger::closeFile() {
  Lock lock(s_mutex);

  if (s_file.is_open())
    s_file.close();
}

void Logger::setHistorySize(std::size_t size) {
  Lock lock(s_mutex);
  if (size == 0) {
    s_histories.reset();
    return;
  }
  UniquePtr<RingBuffer<std::string>> next(new RingBuffer<std::string>(size));
  if (s_histories)
    for (std::size_t i = 0; i < s_histories->size(); ++i)
      next->push((*s_histories)[i]);
  s_histories = std::move(next);
}

std::vector<std::string> Logger::history() {
  Lock lock(s_mutex);
  return historyUnlocked();
}

std::vector<std::string> Logger::historyUnlocked() {
  std::vector<std::string> histories;

  if (s_histories == nullptr)
    return histories;
  std::size_t size = s_histories->size();

  histories.reserve(size);
  for (std::size_t i = 0; i < size; i++)
    histories.push_back(s_histories->at(i));
  return histories;
}
