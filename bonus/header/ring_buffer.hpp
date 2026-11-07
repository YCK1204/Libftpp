#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include "string_format.hpp"
#include <cstddef>
#include <stdexcept>
#include <vector>

template <typename TType>
class RingBuffer {

public:
  enum class OverflowPolicy { OVERWRITE,
                              REJECT };
  explicit RingBuffer(std::size_t capacity, OverflowPolicy policy = OverflowPolicy::OVERWRITE);
  bool push(const TType &value);
  TType pop();
  TType &front();
  TType &back();
  TType &operator[](std::size_t i);
  TType &at(std::size_t i);
  const TType &front() const;
  const TType &back() const;
  const TType &operator[](std::size_t i) const;
  const TType &at(std::size_t i) const;
  std::size_t size() const;
  std::size_t capacity() const;
  bool empty() const;
  bool full() const;
  void clear();
  OverflowPolicy policy() const;

private:
  std::vector<TType> m_buffer;
  std::size_t m_head = 0;
  std::size_t m_size = 0;
  OverflowPolicy m_policy;
};

template <typename TType>
inline RingBuffer<TType>::RingBuffer(std::size_t capacity, OverflowPolicy policy) : m_policy(policy) {
  if (capacity == 0)
    throw std::invalid_argument("RingBuffer: capacity is zero");
  m_buffer.resize(capacity);
}

template <typename TType>
inline bool RingBuffer<TType>::push(const TType &value) {
  if (full()) {
    if (policy() == RingBuffer::OverflowPolicy::REJECT)
      return false;
    pop();
  }
  m_buffer[(m_head + m_size) % m_buffer.size()] = value;
  m_size++;
  return true;
}

template <typename TType>
inline TType RingBuffer<TType>::pop() {
  if (empty())
    throw std::out_of_range("RingBuffer: pop from empty buffer");
  TType f = m_buffer[m_head];
  m_buffer[m_head] = TType();
  m_head = (m_head + 1) % m_buffer.size();
  m_size--;
  return f;
}

template <typename TType>
inline TType &RingBuffer<TType>::front() {
  if (empty())
    throw std::out_of_range("RingBuffer: front from empty buffer");
  return m_buffer[m_head];
}

template <typename TType>
inline TType &RingBuffer<TType>::back() {
  if (empty())
    throw std::out_of_range("RingBuffer: back from empty buffer");
  return m_buffer[(m_head + m_size - 1) % m_buffer.size()];
}

template <typename TType>
inline TType &RingBuffer<TType>::operator[](std::size_t i) {
  return m_buffer[(m_head + i) % m_buffer.size()];
}

template <typename TType>
inline TType &RingBuffer<TType>::at(std::size_t i) {
  if (i >= size())
    throw std::out_of_range("RingBuffer: at (" + StringFormat::toString(i) + ") out of range");
  return m_buffer[(m_head + i) % m_buffer.size()];
}

template <typename TType>
inline const TType &RingBuffer<TType>::front() const {
  if (empty())
    throw std::out_of_range("RingBuffer: front from empty buffer");
  return m_buffer[m_head];
}

template <typename TType>
inline const TType &RingBuffer<TType>::back() const {
  if (empty())
    throw std::out_of_range("RingBuffer: back from empty buffer");
  return m_buffer[(m_head + m_size - 1) % m_buffer.size()];
}

template <typename TType>
inline const TType &RingBuffer<TType>::operator[](std::size_t i) const {
  return m_buffer[(m_head + i) % m_buffer.size()];
}

template <typename TType>
inline const TType &RingBuffer<TType>::at(std::size_t i) const {
  if (i >= size())
    throw std::out_of_range("RingBuffer: at (" + StringFormat::toString(i) + ") out of range");
  return m_buffer[(m_head + i) % m_buffer.size()];
}

template <typename TType>
inline std::size_t RingBuffer<TType>::size() const { return m_size; }

template <typename TType>
inline std::size_t RingBuffer<TType>::capacity() const { return m_buffer.size(); }

template <typename TType>
inline bool RingBuffer<TType>::empty() const { return m_size == 0; }

template <typename TType>
inline bool RingBuffer<TType>::full() const { return size() == capacity(); }

template <typename TType>
inline void RingBuffer<TType>::clear() { 
    for (std::size_t i = 0; i < m_buffer.size(); i++)
        m_buffer[i] = TType();
    m_size = 0;
    m_head = 0;
}

template <typename TType>
inline typename RingBuffer<TType>::OverflowPolicy RingBuffer<TType>::policy() const { return m_policy; }

#endif