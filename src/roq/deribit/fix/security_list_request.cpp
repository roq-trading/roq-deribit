/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/security_list_request.h"

#include "roq/logging.h"

#include "roq/core/fix/security_list_request.h"
#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message SecurityListRequest::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time) const {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::SecurityListRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::SECURITY_REQ_ID, security_req_id)
    .write(
        core::fix::Field::SECURITY_LIST_REQUEST_TYPE,
        security_list_request_type)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
