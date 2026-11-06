#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "define.hpp"
#include "data_buffer.hpp"
#include <ostream>
#include <vector>

// format [ [type:uint32] [size:uint32] [body:vector<uint8>] ]
class Message {
public:
  static const std::size_t SizeOfType = sizeof(uint32);
  static const std::size_t SizeOfBodySize = sizeof(uint32);
  static const std::size_t SizeOfHeader = SizeOfType + SizeOfBodySize;

public:
  typedef int Type;

  explicit Message(Type type);

  template <typename T>
  Message &operator<<(const T &value);
  template <typename T>
  const Message &operator>>(T &value) const;

  const Type &type() const;
  std::vector<uint8> serialize() const;
  static std::size_t deserialize(const std::vector<uint8> &data, Out Message &out);
  static std::size_t deserialize(const std::vector<uint8> &data, std::size_t offset, Out Message &out);

private:
  mutable DataBuffer m_dataBuffer;
  Type m_type;
};

template <typename T>
inline const Message &Message::operator>>(Out T &value) const {
  m_dataBuffer >> value;
  return *this;
}

template <typename T>
inline Message &Message::operator<<(const T &value) {
  m_dataBuffer << value;
  return *this;
}

#endif