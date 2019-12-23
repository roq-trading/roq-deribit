/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/user_request.h"

#include "roq/logging.h"

#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message UserRequest::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time) const {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::UserRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::USER_REQUEST_ID, user_request_id)
    // TODO(thraneh): do *not* hardcode!
    .write(
        core::fix::Field::USER_REQUEST_TYPE,
        core::fix::UserRequestType::REQUEST_INDIVIDUAL_USER_STATUS)
    .write(core::fix::Field::USERNAME, username)
    .write(
        core::fix::Field::SECURITY_LIST_REQUEST_TYPE,
        core::fix::SecurityListRequestType::ALL_SECURITIES)
    .write(core::fix::Field::CURRENCY, currency)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
