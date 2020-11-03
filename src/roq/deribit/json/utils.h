/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/core/utility.h"

#include "roq/core/json/parser.h"

#include "roq/core/charconv/datetime.h"

#include "roq/deribit/json/direction.h"
#include "roq/deribit/json/kind.h"
#include "roq/deribit/json/option_type.h"
#include "roq/deribit/json/state.h"

namespace roq {
namespace deribit {
namespace json {

template <typename T>
inline void update(T &result, const core::json::value_t &value) {
  result = core::json::get<T>(value);
}

template <>
inline void update(
    std::chrono::nanoseconds &result, const core::json::value_t &value) {
  result = std::chrono::milliseconds{core::json::get<uint64_t>(value)};
}

template <>
inline void update(Direction &result, const core::json::value_t &value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

template <>
inline void update(Kind &result, const core::json::value_t &value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

template <>
inline void update(OptionType &result, const core::json::value_t &value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

template <>
inline void update(State &result, const core::json::value_t &value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

// ...

inline TradingStatus map(State state) {
  switch (state) {
    case State::UNDEFINED: break;
    case State::UNKNOWN: break;
    case State::CLOSED: return TradingStatus::OPEN;
    case State::OPEN: return TradingStatus::OPEN;
  }
  return TradingStatus::UNDEFINED;
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
