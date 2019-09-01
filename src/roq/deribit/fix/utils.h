/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "roq/core/charconv/number.h"

namespace roq {
namespace deribit {
namespace fix {

inline void update(
    std::string_view& result,
    const std::string_view& value) {
  result = value;
}

inline void update(
    bool& result,
    const std::string_view& value) {
  if (value.length() > 0) {
    switch (value.data()[0]) {
      case 'Y':
        result = true;
        return;
      case 'N':
        result = false;
        return;
    }
  }
  throw std::runtime_error("Not a valid boolean");
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value, void>::type
inline update(
    T& result,
    const std::string_view& value) {
  result = core::charconv::from_string<T>(value);
}

/*
template <>
inline void update(
    std::chrono::nanoseconds& result,
    const core::fix::value_t& value) {
  result = std::chrono::milliseconds{core::fix::get<uint64_t>(value)};
}
*/

}  // namespace fix
}  // namespace deribit
}  // namespace roq
