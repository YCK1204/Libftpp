#include "data_buffer.hpp"

DataBuffer::DataBuffer() : m_readPos(0) {}

DataBuffer &DataBuffer::operator<<(const char *_string) {
  if (_string == nullptr)
    throw std::invalid_argument("DataBuffer: null string");
  return *this << std::string(_string);
}
DataBuffer &DataBuffer::operator<<(const std::string &_string) {
  std::size_t len = _string.length();
  writeBytes(&len, sizeof(len));
  writeBytes(_string.data(), len);
  return *this;
}

DataBuffer &DataBuffer::operator>>(std::string &_string) {
  std::size_t len;
  readBytes(&len, sizeof(len));
  if (len > m_buffer.size() - m_readPos)
    throw std::out_of_range("DataBuffer: invalid string length");
  _string.assign(reinterpret_cast<const char *>(m_buffer.data() + m_readPos), len);
  m_readPos += len;
  return *this;
}

void DataBuffer::clear() {
  m_readPos = 0;
  m_buffer.clear();
}

std::size_t DataBuffer::getBufferSize() const {
  return m_buffer.size();
}

void DataBuffer::writeBytes(const void *_src, const std::size_t &_size) {
  const uint8 *ptr = reinterpret_cast<const uint8 *>(_src);
  m_buffer.insert(m_buffer.end(), ptr, ptr + _size);
}

void DataBuffer::readBytes(void *_dest, const std::size_t &_size) {
  if (_size > m_buffer.size() - m_readPos)
    throw std::out_of_range("DataBuffer: not enough data to read");
  std::memcpy(_dest, m_buffer.data() + m_readPos, _size);
  m_readPos += _size;
}

const std::vector<uint8> &DataBuffer::getBuffer() const { return m_buffer; }
