/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/request_for_positions.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message RequestForPositions::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(core::fix::Field::POS_REQ_ID, pos_req_id)
    .write(core::fix::Field::POS_REQ_TYPE, pos_req_type)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
