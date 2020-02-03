/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/user_request.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message UserRequest::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(
        core::fix::Field::USER_REQUEST_ID,
        user_request_id)
    // TODO(thraneh): do *not* hardcode!
    .write(
        core::fix::Field::USER_REQUEST_TYPE,
        core::fix::UserRequestType::REQUEST_INDIVIDUAL_USER_STATUS)
    .write(
        core::fix::Field::USERNAME,
        username)
    .write(
        core::fix::Field::SECURITY_LIST_REQUEST_TYPE,
        core::fix::SecurityListRequestType::ALL_SECURITIES)
    .write(
        core::fix::Field::CURRENCY,
        currency)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
