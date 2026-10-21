#pragma once
#include <memory>
#include <cstdint>

#define Out
#define In

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

template <typename T>
using SharedPtr = std::shared_ptr<T>;
template <typename T>
using UniquePtr = std::unique_ptr<T>;
template <typename T>
using WeakPtr = std::weak_ptr<T>;