/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/new_order_single.h"

namespace roq {
namespace deribit {
namespace fix {

constexpr auto PRECISION = size_t{8};

core::utils::Message NewOrderSingle::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(
        core::fix::Field::CL_ORD_ID,
        cl_ord_id)
    .write(
        core::fix::Field::SIDE,
        side)
    .write(
        core::fix::Field::ORDER_QTY,
        order_qty,
        PRECISION)
    .write(
        core::fix::Field::PRICE,
        price,
        PRECISION)
    .write(
        core::fix::Field::SYMBOL,
        symbol)
    .write(
        core::fix::Field::ORD_TYPE,
        ord_type)
    .write(
        core::fix::Field::TIME_IN_FORCE,
        time_in_force)
    .write(
        static_cast<uint32_t>(fix::Deribit::LABEL),
        deribit_label)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
