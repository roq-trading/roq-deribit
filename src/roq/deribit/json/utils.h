/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/core/utility.h"

#include "roq/core/json/parser.h"

#include "roq/core/charconv/datetime.h"

#include "roq/deribit/api/enums.h"

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
  result = core::charconv::to_datetime(
      core::json::get<std::string_view>(value));
}

template <>
inline void update(
    api::Side& result,
    const core::json::value_t& value) {
  result = api::to_enum<std::remove_reference<decltype(result)>::type>(
      core::json::get<std::string_view>(value));
}

template <>
inline void update(
    api::OrderType& result,
    const core::json::value_t& value) {
  result = api::to_enum<std::remove_reference<decltype(result)>::type>(
      core::json::get<std::string_view>(value));
}

template <>
inline void update(
    api::OrderStatus& result,
    const core::json::value_t& value) {
  result = api::to_enum<std::remove_reference<decltype(result)>::type>(
      core::json::get<std::string_view>(value));
}

template <>
inline void update(
    api::TimeInForce& result,
    const core::json::value_t& value) {
  result = api::to_enum<std::remove_reference<decltype(result)>::type>(
      core::json::get<std::string_view>(value));
}

template <>
inline void update(
    api::Reason& result,
    const core::json::value_t& value) {
  result = api::to_enum<std::remove_reference<decltype(result)>::type>(
      core::json::get<std::string_view>(value));
}

template <>
inline void update(
    api::XStatus& result,
    const core::json::value_t& value) {
  result = api::to_enum<std::remove_reference<decltype(result)>::type>(
      core::json::get<std::string_view>(value));
}

template <>
inline void update(
    api::StopType& result,
    const core::json::value_t& value) {
  result = api::to_enum<std::remove_reference<decltype(result)>::type>(
      core::json::get<std::string_view>(value));
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
