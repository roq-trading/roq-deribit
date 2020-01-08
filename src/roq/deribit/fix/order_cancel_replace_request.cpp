/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/order_cancel_replace_request.h"

namespace roq {
namespace deribit {
namespace fix {

constexpr auto PRECISION = size_t{8};

core::utils::Message OrderCancelReplaceRequest::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(core::fix::Field::CL_ORD_ID, cl_ord_id)
    .write(core::fix::Field::ORIG_CL_ORD_ID, orig_cl_ord_id)
    .write(core::fix::Field::TRANSACT_TIME, transact_time)
    .write(core::fix::Field::SIDE, side)
    .write(core::fix::Field::ORDER_QTY, order_qty, PRECISION)
    .write(core::fix::Field::ORD_TYPE, ord_type)
    .write(core::fix::Field::PRICE, price, PRECISION)
    .write(core::fix::Field::SYMBOL, symbol)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
