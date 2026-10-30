#ifndef POOL_HPP
#define POOL_HPP

#include "define.hpp"
#include <cstddef>
#include <new>
#include <utility>
#include <vector>

template <typename TType>
class Pool {
public:
  class Object {
  public:
    ~Object();
    TType *operator->();
    Object(Object &&_other);

  private:
    friend class Pool;
    Object(Pool *_pool, TType *_ptr);
    Pool *m_pool;
    TType *m_ptr;
    Object(const Object &) = delete;
    Object &operator=(const Object &) = delete;
  };

public:
  Pool();
  Pool(std::size_t _size);
  Pool(const Pool &_other) = delete;
  Pool &operator=(const Pool &pool) = delete;
  ~Pool();
  void resize(const std::size_t &_numberOfObjectStored);
  template <typename... TArgs>
  Object acquire(TArgs &&..._args);

private:
  std::vector<TType *> m_storage;
  std::vector<TType *> m_freeSlot;
  void releaseAll();
};

#pragma region Pool
template <typename TType>
inline Pool<TType>::Pool() : Pool(5) {}

template <typename TType>
inline Pool<TType>::Pool(std::size_t _size) { resize(_size); }

template <typename TType>
inline Pool<TType>::~Pool() { releaseAll(); }

template <typename TType>
template <typename... TArgs>
inline typename Pool<TType>::Object Pool<TType>::acquire(TArgs &&..._args) {
  if (m_freeSlot.empty())
    resize(m_storage.empty() ? 1 : m_storage.size() * 2);

  TType *ptr = m_freeSlot.back();
  new (ptr) TType(std::forward<TArgs>(_args)...);
  m_freeSlot.pop_back();

  return Object(this, ptr);
}

template <typename TType>
inline void Pool<TType>::resize(const std::size_t &_numberOfObjectStored) {
  std::size_t curStorageSize = m_storage.size();

  if (curStorageSize >= _numberOfObjectStored)
    return;

  m_storage.reserve(_numberOfObjectStored);
  m_freeSlot.reserve(_numberOfObjectStored);

  for (std::size_t i = curStorageSize; i < _numberOfObjectStored; i++) {
    TType *ptr = static_cast<TType *>(::operator new(sizeof(TType)));
    m_storage.push_back(ptr);
    m_freeSlot.push_back(ptr);
  }
}

template <typename TType>
inline void Pool<TType>::releaseAll() {
  for (std::size_t i = 0; i < m_storage.size(); i++) {
    ::operator delete(m_storage[i]);
  }
  m_storage.clear();
  m_freeSlot.clear();
}
#pragma endregion

#pragma region Object
template <typename TType>
inline Pool<TType>::Object::Object(Pool *_pool, TType *_ptr) {
  m_pool = _pool;
  m_ptr = _ptr;
}

template <typename TType>
inline Pool<TType>::Object::~Object() {
  if (m_ptr == nullptr)
    return;
  m_ptr->~TType();
  m_pool->m_freeSlot.push_back(m_ptr);
}

template <typename TType>
inline TType *Pool<TType>::Object::operator->() {
  return m_ptr;
}
template <typename TType>
inline Pool<TType>::Object::Object(Object &&_other) : m_pool(_other.m_pool), m_ptr(_other.m_ptr) {
  m_pool = nullptr;
  m_ptr = nullptr;
}
#pragma endregion

#endif