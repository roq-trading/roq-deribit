/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/utils/message.h"

#include "roq/core/fix/order_cancel_request.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct OrderCancelRequest final {
  std::string_view cl_ord_id;
  std::string_view orig_cl_ord_id;

  core::utils::Message encode(
      core::utils::Buffer& buffer,
      uint64_t& msg_seq_num,
      std::chrono::nanoseconds sending_time) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::OrderCancelRequest> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::OrderCancelRequest& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "cl_ord_id=\"{}\", "
        "orig_cl_ord_id=\"{}\""
        "}}",
        value.cl_ord_id,
        value.orig_cl_ord_id);
  }
};
