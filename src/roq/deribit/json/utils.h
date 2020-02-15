/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/core/utility.h"

#include "roq/core/json/parser.h"

#include "roq/core/charconv/datetime.h"

#include "roq/deribit/json/kind.h"
#include "roq/deribit/json/option_type.h"
#include "roq/deribit/json/state.h"

namespace roq {
namespace deribit {
namespace json {

template <typename T>
inline void update(
    T& result,
    const core::json::value_t& value) {
  result = core::json::get<T>(value);
}

template <>
inline void update(
    std::chrono::nanoseconds& result,
    const core::json::value_t& value) {
  result = std::chrono::milliseconds{core::json::get<uint64_t>(value)};
}

template <>
inline void update(
    Kind& result,
    const core::json::value_t& value) {
  result = parse_kind(core::json::get<std::string_view>(value));
}

template <>
inline void update(
    OptionType& result,
    const core::json::value_t& value) {
  result = parse_option_type(core::json::get<std::string_view>(value));
}

template <>
inline void update(
    State& result,
    const core::json::value_t& value) {
  result = parse_state(core::json::get<std::string_view>(value));
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
