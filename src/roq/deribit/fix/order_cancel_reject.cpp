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

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::OrderCancelReject::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

template <auto field>
constexpr void non_standard_field() {
  static_assert(has_field(field) == false);
}

void update_field(
    auto& result,
    auto& iter) {
  auto& [tag, value] = *iter;
  try {
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::CL_ORD_ID:
        check_field<core::fix::Field::CL_ORD_ID>();
        core::fix::update(result.cl_ord_id, value);
        break;
      case core::fix::Field::ORD_STATUS:
        check_field<core::fix::Field::ORD_STATUS>();
        core::fix::update(result.ord_status, value);
        break;
      case core::fix::Field::ORIG_CL_ORD_ID:
        check_field<core::fix::Field::ORIG_CL_ORD_ID>();
        core::fix::update(result.orig_cl_ord_id, value);
        break;
      case core::fix::Field::TEXT:
        check_field<core::fix::Field::TEXT>();
        core::fix::update(result.text, value);
        break;
      // non-standard
      case core::fix::Field::AVG_PX:
        non_standard_field<core::fix::Field::AVG_PX>();
        core::fix::update(result.avg_px, value);
        break;
      case core::fix::Field::LEAVES_QTY:
        non_standard_field<core::fix::Field::LEAVES_QTY>();
        core::fix::update(result.leaves_qty, value);
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
}  // namespace

OrderCancelReject OrderCancelReject::create(
    const core::fix::message_t& message) {
  OrderCancelReject result;
  for (auto iter = message.begin(); iter != message.end(); ++iter)
    update_field(result, iter);
  return result;
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
