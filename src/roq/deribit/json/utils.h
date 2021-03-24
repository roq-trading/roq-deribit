/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/core/utility.h"

#include "roq/core/json/parser.h"

#include "roq/core/charconv/datetime.h"

#include "roq/deribit/json/direction.h"
#include "roq/deribit/json/kind.h"
#include "roq/deribit/json/option_type.h"
#include "roq/deribit/json/order_state.h"
#include "roq/deribit/json/order_type.h"
#include "roq/deribit/json/state.h"
#include "roq/deribit/json/time_in_force.h"

namespace roq {
namespace deribit {
namespace json {

template <typename T>
inline void update(T &result, const core::json::value_t &value) {
  result = core::json::get<T>(value);
}

template <>
inline void update(std::chrono::milliseconds &result, const core::json::value_t &value) {
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
inline void update(OrderState &result, const core::json::value_t &value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

template <>
inline void update(OrderType &result, const core::json::value_t &value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

template <>
inline void update(State &result, const core::json::value_t &value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

template <>
inline void update(TimeInForce &result, const core::json::value_t &value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

// ...

inline Side map(Direction direction) {
  switch (direction) {
    case Direction::UNDEFINED:
      break;
    case Direction::UNKNOWN:
      break;
    case Direction::BUY:
      return Side::BUY;
    case Direction::SELL:
      return Side::SELL;
  }
  return Side::UNDEFINED;
}

inline TradingStatus map(State state) {
  switch (state) {
    case State::UNDEFINED:
      break;
    case State::UNKNOWN:
      break;
    case State::CLOSED:
      return TradingStatus::OPEN;
    case State::OPEN:
      return TradingStatus::OPEN;
    case State::CREATED:     // XXX don't know how to map
    case State::SETTLED:     // XXX don't know how to map
    case State::TERMINATED:  // XXX don't know how to map
      break;
  }
  return TradingStatus::UNDEFINED;
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
