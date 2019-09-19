/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/new_order_single.h"

#include "roq/logging.h"

#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message NewOrderSingle::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time) const {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::NewOrderSingle::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::CL_ORD_ID, cl_ord_id)
    .write(core::fix::Field::SIDE, side)
    .write(core::fix::Field::ORDER_QTY, order_qty)
    .write(core::fix::Field::PRICE, price)
    .write(core::fix::Field::SYMBOL, symbol)
    .write(core::fix::Field::ORD_TYPE, ord_type)
    .write(core::fix::Field::TIME_IN_FORCE, time_in_force)
    .write(static_cast<uint32_t>(fix::Deribit::LABEL), deribit_label)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
