/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "roq/core/charconv/number.h"

#include "roq/core/fix/common.h"

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
typename std::enable_if<
    std::is_integral<T>::value || std::is_floating_point<T>::value,
    void
    >::type
inline update(
    T& result,
    const std::string_view& value) {
  result = core::charconv::from_string<T>(value);
}

inline void update(
    std::chrono::nanoseconds& result,
    const std::string_view& value) {
  // result = std::chrono::milliseconds{core::fix::get<uint64_t>(value)};
}

inline void update(
    core::fix::MDEntryType& result,
    const std::string_view& value) {
  result = core::fix::parse_md_entry_type(value);
}

inline void update(
    core::fix::OrdStatus& result,
    const std::string_view& value) {
  // result = core::fix::parse_ord_status(value);
}

inline void update(
    core::fix::OrdType& result,
    const std::string_view& value) {
  // result = core::fix::parse_ord_type(value);
}

inline void update(
    core::fix::QtyType& result,
    const std::string_view& value) {
  // result = core::fix::parse_qty_type(value);
}

inline void update(
    core::fix::Side& result,
    const std::string_view& value) {
  // result = core::fix::parse_side(value);
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
