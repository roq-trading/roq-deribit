/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <limits>

#include "roq/core/utils/message.h"

#include "roq/core/fix/new_order_single.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct NewOrderSingle final {
  std::string_view cl_ord_id;
  core::fix::Side side = core::fix::Side::UNKNOWN;
  double order_qty = std::numeric_limits<double>::quiet_NaN();
  double price = std::numeric_limits<double>::quiet_NaN();
  std::string_view symbol;
  core::fix::OrdType ord_type = core::fix::OrdType::UNKNOWN;
  core::fix::TimeInForce time_in_force = core::fix::TimeInForce::UNKNOWN;
  std::string_view deribit_label;

  core::utils::Message encode(
      core::utils::Buffer& buffer,
      uint64_t& msg_seq_num,
      std::chrono::nanoseconds sending_time) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq
