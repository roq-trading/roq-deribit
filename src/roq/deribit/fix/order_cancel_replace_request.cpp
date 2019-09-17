/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/order_cancel_replace_request.h"

#include "roq/logging.h"

#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message OrderCancelReplaceRequest::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time,
    const std::string_view& cl_ord_id,
    const std::string_view& orig_cl_ord_id,
    const core::fix::Side& side,
    double order_qty,
    const core::fix::OrdType& ord_type,
    double price,
    const std::string_view& symbol,
    std::chrono::nanoseconds transact_time) {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::OrderCancelReplaceRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::CL_ORD_ID, cl_ord_id)
    .write(core::fix::Field::ORIG_CL_ORD_ID, orig_cl_ord_id)
    .write(core::fix::Field::TRANSACT_TIME, transact_time)
    .write(core::fix::Field::SIDE, side)
    .write(core::fix::Field::ORDER_QTY, order_qty)
    .write(core::fix::Field::ORD_TYPE, ord_type)
    .write(core::fix::Field::PRICE, price)
    .write(core::fix::Field::SYMBOL, symbol)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
