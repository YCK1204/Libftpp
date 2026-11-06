#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <memory>
#include <stdexcept>
#include <utility>

template <typename TType>
class Singleton {
public:
  template <typename... TArgs>
  static void instantiate(TArgs &&...p_args);
  static TType *instance();

private:
  static std::unique_ptr<TType> m_instance;
};
template <typename TType>
inline TType *Singleton<TType>::instance() { return m_instance.get(); }

template <typename TType>
template <typename... TArgs>
inline void Singleton<TType>::instantiate(TArgs &&...p_args) {
  if (m_instance != nullptr) {
    throw std::runtime_error("Singleton: instance already created");
  }

  m_instance.reset(new TType(std::forward<TArgs>(p_args)...));
}

template <typename TType>
std::unique_ptr<TType> Singleton<TType>::m_instance = nullptr;

#endif