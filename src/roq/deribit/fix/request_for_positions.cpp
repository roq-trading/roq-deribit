/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/request_for_positions.h"

#include "roq/logging.h"

#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message RequestForPositions::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time) const {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::RequestForPositions::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::POS_REQ_ID, pos_req_id)
    .write(core::fix::Field::POS_REQ_TYPE, pos_req_type)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
