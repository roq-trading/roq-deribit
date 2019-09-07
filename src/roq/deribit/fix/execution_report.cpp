/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/execution_report.h"

#include "roq/logging.h"

#include "roq/core/fix/execution_report.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

ExecutionReport ExecutionReport::parse(
    const core::fix::message_t& message) {
  ExecutionReport result;
  parse(result, message);
  return result;
}

void ExecutionReport::parse(
    ExecutionReport& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void ExecutionReport::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::AVG_PX:
          core::fix::update(avg_px, value);
          break;
        case core::fix::Field::CL_ORD_ID:
          core::fix::update(cl_ord_id, value);
          break;
        case core::fix::Field::CONTRACT_MULTIPLIER:
          core::fix::update(contract_multiplier, value);
          break;
        case core::fix::Field::CUM_QTY:
          core::fix::update(cum_qty, value);
          break;
        case core::fix::Field::EXEC_INST:
          core::fix::update(exec_inst, value);
          break;
        case core::fix::Field::LEAVES_QTY:
          core::fix::update(leaves_qty, value);
          break;
        case core::fix::Field::MAX_SHOW:
          core::fix::update(max_show, value);
          break;
        case core::fix::Field::ORD_REJ_REASON:
          core::fix::update(ord_rej_reason, value);
          break;
        case core::fix::Field::ORD_STATUS:
          core::fix::update(ord_status, value);
          break;
        case core::fix::Field::ORD_TYPE:
          core::fix::update(ord_type, value);
          break;
        case core::fix::Field::ORDER_QTY:
          core::fix::update(order_qty, value);
          break;
        case core::fix::Field::ORIG_CL_ORD_ID:
          core::fix::update(orig_cl_ord_id, value);
          break;
        case core::fix::Field::PEGGED_PRICE:
          core::fix::update(pegged_price, value);
          break;
        case core::fix::Field::PRICE:
          core::fix::update(price, value);
          break;
        case core::fix::Field::QTY_TYPE:
          core::fix::update(qty_type, value);
          break;
        case core::fix::Field::SECURITY_EXCHANGE:
          core::fix::update(security_exchange, value);
          break;
        case core::fix::Field::SIDE:
          core::fix::update(side, value);
          break;
        case core::fix::Field::STOP_PX:
          core::fix::update(stop_px, value);
          break;
        case core::fix::Field::SYMBOL:
          core::fix::update(symbol, value);
          break;
        case core::fix::Field::TEXT:
          core::fix::update(text, value);
          break;
        case core::fix::Field::TRANSACT_TIME:
          core::fix::update(transact_time, value);
          break;
        case core::fix::Field::VOLATILITY:
          core::fix::update(volatility, value);
          break;
        default:
          if (core::fix::ExecutionReport::has_field(field))
            break;
          switch (static_cast<Deribit>(tag)) {
            case Deribit::ADV_ORDER_TYPE:
              update(deribit_adv_order_type, value);
              break;
            default:
              throw std::runtime_error(
                  fmt::format(
                      "Unknown field: tag={} field={} value=\"{}\"",
                      tag, field, value));
          }
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
