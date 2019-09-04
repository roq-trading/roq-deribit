/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "roq/core/charconv/number.h"

#include "roq/core/fix/common.h"
#include "roq/core/fix/charconv.h"

#include "roq/deribit/fix/deribit.h"

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
  result = core::fix::parse_utc_timestamp(value);
}

inline void update(
    core::fix::MDEntryType& result,
    const std::string_view& value) {
  result = core::fix::parse_md_entry_type(value);
}

inline void update(
    core::fix::MDUpdateAction& result,
    const std::string_view& value) {
  result = core::fix::parse_md_update_action(value);
}

inline void update(
    core::fix::MDUpdateType& result,
    const std::string_view& value) {
  result = core::fix::parse_md_update_type(value);
}

inline void update(
    core::fix::MDReqRejReason& result,
    const std::string_view& value) {
  result = core::fix::parse_md_req_rej_reason(value);
}

inline void update(
    core::fix::OrdStatus& result,
    const std::string_view& value) {
  result = core::fix::parse_ord_status(value);
}

inline void update(
    core::fix::OrdType& result,
    const std::string_view& value) {
  result = core::fix::parse_ord_type(value);
}

inline void update(
    core::fix::QtyType& result,
    const std::string_view& value) {
  result = core::fix::parse_qty_type(value);
}

inline void update(
    core::fix::Side& result,
    const std::string_view& value) {
  result = core::fix::parse_side(value);
}

inline void update(
    core::fix::SecurityRequestResult& result,
    const std::string_view& value) {
  result = core::fix::parse_security_request_result(value);
}

inline void update(
    core::fix::PosReqResult& result,
    const std::string_view& value) {
  result = core::fix::parse_pos_req_result(value);
}

inline void update(
    core::fix::PosReqType& result,
    const std::string_view& value) {
  result = core::fix::parse_pos_req_type(value);
}

inline void update(
    core::fix::PutOrCall& result,
    const std::string_view& value) {
  result = core::fix::parse_put_or_call(value);
}

inline void update(
    core::fix::SettlType& result,
    const std::string_view& value) {
  result = core::fix::parse_settl_type(value);
}

inline void update(
    AdvOrderType& result,
    const std::string_view& value) {
  result = parse_adv_order_type(value);
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
