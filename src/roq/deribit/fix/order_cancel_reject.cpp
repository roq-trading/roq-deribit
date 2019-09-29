/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/order_cancel_reject.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/order_cancel_reject.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

OrderCancelReject OrderCancelReject::parse(
    const core::fix::message_t& message) {
  OrderCancelReject result;
  parse(result, message);
  return result;
}

void OrderCancelReject::parse(
    OrderCancelReject& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void OrderCancelReject::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::CL_ORD_ID:
          static_assert(core::fix::OrderCancelReject::has_field(core::fix::Field::CL_ORD_ID));
          core::fix::update(cl_ord_id, value);
          break;
        case core::fix::Field::ORD_STATUS:
          static_assert(core::fix::OrderCancelReject::has_field(core::fix::Field::ORD_STATUS));
          core::fix::update(ord_status, value);
          break;
        case core::fix::Field::ORIG_CL_ORD_ID:
          static_assert(core::fix::OrderCancelReject::has_field(core::fix::Field::ORIG_CL_ORD_ID));
          core::fix::update(orig_cl_ord_id, value);
          break;
        case core::fix::Field::TEXT:
          static_assert(core::fix::OrderCancelReject::has_field(core::fix::Field::TEXT));
          core::fix::update(text, value);
          break;
        // non-standard
        case core::fix::Field::AVG_PX:
          static_assert(!core::fix::OrderCancelReject::has_field(core::fix::Field::AVG_PX));
          core::fix::update(avg_px, value);
          break;
        case core::fix::Field::LEAVES_QTY:
          static_assert(!core::fix::OrderCancelReject::has_field(core::fix::Field::LEAVES_QTY));
          core::fix::update(leaves_qty, value);
          break;
        default:
          if (core::fix::OrderCancelReject::has_field(field))
            break;
          throw core::fix::InvalidField(
              "OrderCancelReject: "
              "Unexpected field={}", tag);
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(
          "OrderCancelReject: "
          "Parse error: "
          "field={}, value=\"{}\", what=\"{}\"",
          tag, value, e.what());
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
