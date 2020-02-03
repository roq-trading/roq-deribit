/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <limits>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct OrderCancelReject final {
  std::string_view cl_ord_id;
  core::fix::OrdStatus ord_status = core::fix::OrdStatus::UNKNOWN;
  std::string_view orig_cl_ord_id;
  std::string_view text;
  // non-standard
  double avg_px = std::numeric_limits<double>::quiet_NaN();
  double leaves_qty = std::numeric_limits<double>::quiet_NaN();

 public:
  OrderCancelReject() = default;
  OrderCancelReject(OrderCancelReject&&) = default;
  OrderCancelReject(const OrderCancelReject&) = delete;

  static OrderCancelReject create(const core::fix::message_t& message);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::OrderCancelReject> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::OrderCancelReject& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "cl_ord_id=\"{}\", "
        "ord_status={}, "
        "orig_cl_ord_id=\"{}\", "
        "text=\"{}\", "
        // non-standard
        "avg_px={}, "
        "leaves_qty={}"
        "}}",
        value.cl_ord_id,
        value.ord_status,
        value.orig_cl_ord_id,
        value.text,
        // non-standard
        value.avg_px,
        value.leaves_qty);
  }
};
