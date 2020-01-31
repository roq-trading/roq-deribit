/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/order_cancel_reject.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/order_cancel_reject.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

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

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::OrderCancelReject::has_field(field);
}
}  // namespace

void OrderCancelReject::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::CL_ORD_ID:
          static_assert(has_field(core::fix::Field::CL_ORD_ID));
          core::fix::update(cl_ord_id, value);
          break;
        case core::fix::Field::ORD_STATUS:
          static_assert(has_field(core::fix::Field::ORD_STATUS));
          core::fix::update(ord_status, value);
          break;
        case core::fix::Field::ORIG_CL_ORD_ID:
          static_assert(has_field(core::fix::Field::ORIG_CL_ORD_ID));
          core::fix::update(orig_cl_ord_id, value);
          break;
        case core::fix::Field::TEXT:
          static_assert(has_field(core::fix::Field::TEXT));
          core::fix::update(text, value);
          break;
        // non-standard
        case core::fix::Field::AVG_PX:
          static_assert(!has_field(core::fix::Field::AVG_PX));
          core::fix::update(avg_px, value);
          break;
        case core::fix::Field::LEAVES_QTY:
          static_assert(!has_field(core::fix::Field::LEAVES_QTY));
          core::fix::update(leaves_qty, value);
          break;
        default:
          if (has_field(field)) {
            DLOG(FATAL)("Unexpected tag={} field={}", tag, field);
            break;
          }
          DLOG(FATAL)("Unknown tag={} field={}", tag, field);
          throw core::fix::InvalidField(tag, value);
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(tag, value, e);
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
