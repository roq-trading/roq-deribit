/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/order_cancel_request.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message OrderCancelRequest::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(
        core::fix::Field::CL_ORD_ID,
        cl_ord_id)
    .write(
        core::fix::Field::ORIG_CL_ORD_ID,
        orig_cl_ord_id)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
