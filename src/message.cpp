#include "message.hpp"
#include <cstring>

Message::Message(Type type) {
  m_type = type;
}

const Message::Type &Message::type() const { return m_type; }
std::vector<uint8> Message::serialize() const {
  std::vector<uint8> ret;

  const std::vector<uint8> &buffer = m_dataBuffer.getBuffer();
  uint32 bodySize = buffer.size();

  ret.assign(SizeOfHeader + bodySize, 0);
  std::size_t pos = 0;
  std::memcpy(ret.data() + pos, &m_type, SizeOfType);
  pos += SizeOfType;
  std::memcpy(ret.data() + pos, &bodySize, SizeOfBodySize);
  pos += SizeOfBodySize;
  if (bodySize > 0)
    std::memcpy(ret.data() + pos, buffer.data(), bodySize);
  return ret;
}

std::size_t Message::deserialize(const std::vector<uint8> &data, Out Message &out) {
  return deserialize(data, 0, out);
}

std::size_t Message::deserialize(const std::vector<uint8> &data, std::size_t offset, Out Message &out) {
  uint32 type, bodySize;

  if (data.size() < offset || data.size() - offset < SizeOfHeader)
    return 0;

  std::memcpy(&type, data.data() + offset, SizeOfType);
  offset += SizeOfType;
  std::memcpy(&bodySize, data.data() + offset, SizeOfBodySize);
  offset += SizeOfBodySize;
  if (data.size() - offset < bodySize)
    return 0;

  out = Message(type);
  out.m_dataBuffer.writeBytes(data.data() + offset, bodySize);
  return SizeOfHeader + bodySize;
}