/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/order_cancel_reject.h"

#include "roq/logging.h"

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
          core::fix::update(cl_ord_id, value);
          break;
        case core::fix::Field::ORD_STATUS:
          core::fix::update(ord_status, value);
          break;
        case core::fix::Field::ORIG_CL_ORD_ID:
          core::fix::update(orig_cl_ord_id, value);
          break;
        case core::fix::Field::TEXT:
          core::fix::update(text, value);
          break;
        default:
          if (core::fix::OrderCancelReject::has_field(field))
            break;
          throw std::runtime_error(
              fmt::format(
                  "Unknown field: tag={} field={} value=\"{}\"",
                  tag, field, value));
      }
    } catch (std::exception& e) {
      LOG(WARNING) << fmt::format(
          "Can't parse tag={} value=\"{}\"", tag, value);
      throw;
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
