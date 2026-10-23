#ifndef DATA_BUFFER_HPP
#define DATA_BUFFER_HPP

#include "Define.hpp"
#include <cstddef>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

class DataBuffer {
private:
  std::vector<uint8> m_buffer;
  std::size_t m_readPos;

public:
  DataBuffer();

public:
  template <typename TType>
  DataBuffer &operator<<(const TType &_t);
  template <typename TType>
  DataBuffer &operator>>(TType &_t);

  DataBuffer &operator<<(const char *_string);
  DataBuffer &operator<<(const std::string &_string);
  DataBuffer &operator>>(std::string &_string);

  template <typename TType>
  DataBuffer &operator<<(const std::vector<TType> &_vec);
  template <typename TType>
  DataBuffer &operator>>(std::vector<TType> &_vec);

  template <typename TKey, typename TValue>
  DataBuffer &operator<<(const std::map<TKey, TValue> &_map);
  template <typename TKey, typename TValue>
  DataBuffer &operator>>(std::map<TKey, TValue> &_map);

  template <typename TKey, typename TValue>
  DataBuffer &operator<<(const std::unordered_map<TKey, TValue> &_map);
  template <typename TKey, typename TValue>
  DataBuffer &operator>>(std::unordered_map<TKey, TValue> &_map);

  template <typename TType>
  DataBuffer &operator<<(const std::set<TType> &_set);
  template <typename TType>
  DataBuffer &operator>>(std::set<TType> &_set);

public:
  void clear();
  std::size_t getBufferSize() const;
  void writeBytes(const void *_src, const std::size_t &_size);
  void readBytes(void *_dest, const std::size_t &_size);
  const std::vector<uint8> &getBuffer() const;
};


#pragma region template

#pragma region set
template <typename TType>
inline DataBuffer &DataBuffer::operator>>(std::set<TType> &_set) {
  std::size_t count;

  *this >> count;

  const std::size_t minElemSize = std::is_trivially_copyable<TType>::value ? sizeof(TType) : 1;
  if (count > (m_buffer.size() - m_readPos) / minElemSize)
    throw std::out_of_range("DataBuffer: invalid set size");
  std::set<TType> result;
  for (std::size_t i = 0; i < count; i++) {
    TType t;
    *this >> t;
    result.insert(t);
  }
  _set.swap(result);
  return *this;
}

template <typename TType>
inline DataBuffer &DataBuffer::operator<<(const std::set<TType> &_set) {
  std::size_t count = _set.size();
  *this << count;

  for (auto it = _set.begin(); it != _set.end(); it++) {
    *this << *it;
  }

  return *this;
}
#pragma endregion

#pragma region unordered_map

template <typename TKey, typename TValue>
inline DataBuffer &DataBuffer::operator>>(std::unordered_map<TKey, TValue> &_map) {
  std::size_t count;

  *this >> count;
  const std::size_t minKeySize = std::is_trivially_copyable<TKey>::value ? sizeof(TKey) : 1;
  const std::size_t minValueSize = std::is_trivially_copyable<TValue>::value ? sizeof(TValue) : 1;
  if (count > (m_buffer.size() - m_readPos) / (minKeySize + minValueSize))
    throw std::out_of_range("DataBuffer: invalid unordered_map size");
  std::unordered_map<TKey, TValue> result;
  for (std::size_t i = 0; i < count; i++) {
    TKey key;
    TValue value;
    *this >> key;
    *this >> value;
    result[key] = value;
  }

  _map.swap(result);
  return *this;
}

template <typename TKey, typename TValue>
inline DataBuffer &DataBuffer::operator<<(const std::unordered_map<TKey, TValue> &_map) {
  std::size_t count = _map.size();

  *this << count;
  for (auto it = _map.begin(); it != _map.end(); it++) {
    *this << it->first;
    *this << it->second;
  }

  return *this;
}
#pragma endregion

#pragma region map
template <typename TKey, typename TValue>
inline DataBuffer &DataBuffer::operator<<(const std::map<TKey, TValue> &_map) {
  std::size_t count = _map.size();
  *this << count;

  for (auto it = _map.begin(); it != _map.end(); it++) {
    *this << it->first;
    *this << it->second;
  }
  return *this;
}

template <typename TKey, typename TValue>
inline DataBuffer &DataBuffer::operator>>(std::map<TKey, TValue> &_map) {
  std::size_t count;

  *this >> count;

  const std::size_t minKeySize = std::is_trivially_copyable<TKey>::value ? sizeof(TKey) : 1;
  const std::size_t minValueSize = std::is_trivially_copyable<TValue>::value ? sizeof(TValue) : 1;
  if (count > (m_buffer.size() - m_readPos) / (minKeySize + minValueSize))
    throw std::out_of_range("DataBuffer: invalid map size");

  std::map<TKey, TValue> result;
  for (std::size_t i = 0; i < count; i++) {
    TKey key;
    TValue value;
    *this >> key;
    *this >> value;
    result[key] = value;
  }

  result.swap(_map);
  return *this;
}
#pragma endregion

#pragma region vector
template <typename TType>
DataBuffer &DataBuffer::operator<<(const std::vector<TType> &_vec) {
  std::size_t count = _vec.size();
  *this << count;
  for (std::size_t i = 0; i < count; i++)
    *this << _vec[i];
  return *this;
}

template <typename TType>
DataBuffer &DataBuffer::operator>>(std::vector<TType> &_vec) {
  std::size_t count;
  *this >> count;

  const std::size_t minElemSize = std::is_trivially_copyable<TType>::value ? sizeof(TType) : 1;
  if (count > (m_buffer.size() - m_readPos) / minElemSize)
    throw std::out_of_range("DataBuffer: invalid vector size");

  std::vector<TType> result;
  result.reserve(count);
  for (std::size_t i = 0; i < count; i++) {
    TType t;
    *this >> t;
    result.push_back(t);
  }
  _vec.swap(result);
  return *this;
}

#pragma endregion

#pragma region value type
template <typename TType>
DataBuffer &DataBuffer::operator<<(const TType &_t) {
  static_assert(std::is_trivially_copyable<TType>::value, "DataBuffer: provide an operator<< overload for this type");
  writeBytes(&_t, sizeof(_t));
  return *this;
}

template <typename TType>
DataBuffer &DataBuffer::operator>>(TType &_t) {
  static_assert(std::is_trivially_copyable<TType>::value, "DataBuffer: provide an operator>> overload for this type");
  readBytes(&_t, sizeof(_t));
  return *this;
}
#pragma endregion

#pragma endregion

#endif