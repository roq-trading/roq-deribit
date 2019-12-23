/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/order_mass_status_request.h"

#include "roq/logging.h"

#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message OrderMassStatusRequest::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time) const {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::OrderMassStatusRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::MASS_STATUS_REQ_ID, mass_status_req_id)
    .write(core::fix::Field::MASS_STATUS_REQ_TYPE, mass_status_req_type)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
