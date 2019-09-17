/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/utils/message.h"

#include "roq/core/fix/order_mass_status_request.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct OrderMassStatusRequest final {
  static core::utils::Message encode(
      core::utils::Buffer& buffer,
      uint64_t& msg_seq_num,
      std::chrono::nanoseconds sending_time,
      const std::string_view& mass_status_req_id,
      const core::fix::MassStatusReqType& mass_status_req_type);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq
