/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/security_list_request.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message SecurityListRequest::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(
        core::fix::Field::SECURITY_REQ_ID,
        security_req_id)
    .write(
        core::fix::Field::SECURITY_LIST_REQUEST_TYPE,
        security_list_request_type)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
