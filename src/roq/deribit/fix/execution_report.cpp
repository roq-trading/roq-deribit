/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/execution_report.h"

#include "roq/logging.h"

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
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::AVG_PX:
        update(avg_px, value);
        break;
      case core::fix::Field::CL_ORD_ID:
        update(cl_ord_id, value);
        break;
      case core::fix::Field::CONTRACT_MULTIPLIER:
        update(contract_multiplier, value);
        break;
      case core::fix::Field::CUM_QTY:
        update(cum_qty, value);
        break;
      case core::fix::Field::EXEC_INST:
        update(exec_inst, value);
        break;
      case core::fix::Field::LEAVES_QTY:
        update(leaves_qty, value);
        break;
      case core::fix::Field::MAX_SHOW:
        update(max_show, value);
        break;
      case core::fix::Field::ORD_REJ_REASON:
        update(ord_rej_reason, value);
        break;
      case core::fix::Field::ORD_STATUS:
        update(ord_status, value);
        break;
      case core::fix::Field::ORD_TYPE:
        update(ord_type, value);
        break;
      case core::fix::Field::ORDER_QTY:
        update(order_qty, value);
        break;
      case core::fix::Field::ORIG_CL_ORD_ID:
        update(orig_cl_ord_id, value);
        break;
      case core::fix::Field::PEGGED_PRICE:
        update(pegged_price, value);
        break;
      case core::fix::Field::PRICE:
        update(price, value);
        break;
      case core::fix::Field::QTY_TYPE:
        update(qty_type, value);
        break;
      case core::fix::Field::SECURITY_EXCHANGE:
        update(security_exchange, value);
        break;
      case core::fix::Field::SIDE:
        update(side, value);
        break;
      case core::fix::Field::STOP_PX:
        update(stop_px, value);
        break;
      case core::fix::Field::SYMBOL:
        update(symbol, value);
        break;
      case core::fix::Field::TEXT:
        update(text, value);
        break;
      case core::fix::Field::TRANSACT_TIME:
        update(transact_time, value);
        break;
      case core::fix::Field::VOLATILITY:
        update(volatility, value);
        break;
      default:
        switch (static_cast<Deribit>(tag)) {
          case Deribit::ADV_ORDER_TYPE:
            update(deribit_adv_order_type, value);
            break;
          default:
            LOG(WARNING) << fmt::format(
                "Unknown field: tag={} field={} value=\"{}\"",
                tag,
                field,
                value);
        }
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
