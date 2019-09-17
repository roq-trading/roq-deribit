/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/order_cancel_request.h"

#include "roq/logging.h"

#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message OrderCancelRequest::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time,
    const std::string_view& cl_ord_id,
    const std::string_view& orig_cl_ord_id) {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::OrderCancelRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::CL_ORD_ID, cl_ord_id)
    .write(core::fix::Field::ORIG_CL_ORD_ID, orig_cl_ord_id)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
