/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <limits>

#include "roq/core/utils/message.h"

#include "roq/core/fix/order_cancel_replace_request.h"
#include "roq/core/fix/writer.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct OrderCancelReplaceRequest final {
  const std::string_view cl_ord_id;
  const std::string_view orig_cl_ord_id;
  const core::fix::Side side = core::fix::Side::UNKNOWN;
  double order_qty = std::numeric_limits<double>::quiet_NaN();
  const core::fix::OrdType ord_type = core::fix::OrdType::UNKNOWN;
  double price = std::numeric_limits<double>::quiet_NaN();
  const std::string_view symbol;
  std::chrono::nanoseconds transact_time = {};

 public:
  static constexpr auto msg_type =
    core::fix::OrderCancelReplaceRequest ::msg_type;

  OrderCancelReplaceRequest() = default;
  OrderCancelReplaceRequest(OrderCancelReplaceRequest&&) = default;
  OrderCancelReplaceRequest(const OrderCancelReplaceRequest&) = delete;

  core::utils::Message encode(core::fix::Writer& writer) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::OrderCancelReplaceRequest> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::OrderCancelReplaceRequest& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "cl_ord_id=\"{}\", "
        "orig_cl_ord_id=\"{}\", "
        "side={}, "
        "order_qty={}, "
        "ord_type={}, "
        "price={}, "
        "symbol=\"{}\", "
        "transact_time={}"
        "}}",
        value.cl_ord_id,
        value.orig_cl_ord_id,
        value.side,
        value.order_qty,
        value.ord_type,
        value.price,
        value.symbol,
        value.symbol,
        value.transact_time);
  }
};
