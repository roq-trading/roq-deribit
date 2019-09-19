/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/utils/message.h"

#include "roq/core/fix/order_cancel_replace_request.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct OrderCancelReplaceRequest final {
  const std::string_view cl_ord_id;
  const std::string_view orig_cl_ord_id;
  const core::fix::Side side;
  double order_qty;
  const core::fix::OrdType ord_type;
  double price;
  const std::string_view symbol;
  std::chrono::nanoseconds transact_time;

  core::utils::Message encode(
      core::utils::Buffer& buffer,
      uint64_t& msg_seq_num,
      std::chrono::nanoseconds sending_time) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq
