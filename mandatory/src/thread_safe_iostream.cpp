#include "thread_safe_iostream.hpp"

std::mutex ThreadSafeIOStream::m_mutex;
thread_local ThreadSafeIOStream threadSafeCout;

void ThreadSafeIOStream::setPrefix(const std::string &prefix) {
  m_prefix = prefix;
}

ThreadSafeIOStream::~ThreadSafeIOStream() {
  std::string rest = m_buffer.str();

  if (!rest.empty()) {
    Lock lock(m_mutex);
    std::cout << m_prefix << rest << std::endl;
  }
}

void ThreadSafeIOStream::tryFlush() {
  std::string str = m_buffer.str();
  std::size_t curPos = 0;
  std::size_t start = 0;
  std::string content;

  while ((curPos = str.find('\n', curPos)) != std::string::npos) {
    content += (m_prefix + str.substr(start, curPos - start) + "\n");
    start = curPos + 1;
    curPos = start;
  }

  if (!content.empty()) {
    Lock lock(m_mutex);
    std::cout << content << std::flush;
  }
  m_buffer.str(str.substr(start));
  m_buffer.clear();
  m_buffer.seekp(0, std::ios::end);
}

ThreadSafeIOStream &ThreadSafeIOStream::operator<<(std::ostream &(*manip)(std::ostream &)) {
  manip(m_buffer);
  tryFlush();
  return *this;
}