/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/order_mass_status_request.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message OrderMassStatusRequest::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(core::fix::Field::MASS_STATUS_REQ_ID, mass_status_req_id)
    .write(core::fix::Field::MASS_STATUS_REQ_TYPE, mass_status_req_type)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
