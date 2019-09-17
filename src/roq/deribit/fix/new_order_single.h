/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/utils/message.h"

#include "roq/core/fix/new_order_single.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct NewOrderSingle final {
  static core::utils::Message encode(
      core::utils::Buffer& buffer,
      uint64_t& msg_seq_num,
      std::chrono::nanoseconds sending_time,
      const std::string_view& cl_ord_id,
      const core::fix::Side& side,
      double order_qty,
      double price,
      const std::string_view& symbol,
      const core::fix::OrdType& ord_type,
      const core::fix::TimeInForce& time_in_force,
      const std::string_view& deribit_label);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq
