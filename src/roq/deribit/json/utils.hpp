/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/utils/patterns.hpp"

#include "roq/core/json/parser.hpp"

namespace roq {
namespace deribit {
namespace json {

template <typename T>
inline void update(T &result, core::json::Value const &value) {
  result = core::json::get<T>(value);
}

template <>
inline void update(double &result, core::json::Value const &value) {
  using namespace std::literals;
  if (std::holds_alternative<std::string_view>(value)) {
    if (std::get<std::string_view>(value) == "undefined"sv) {
      result = NaN;
      return;
    }
  }
  result = core::json::get<double>(value);
}

template <>
inline void update(std::chrono::milliseconds &result, core::json::Value const &value) {
  using result_type = std::remove_cvref_t<decltype(result)>;
  result = result_type{core::json::get<uint64_t>(value)};
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
