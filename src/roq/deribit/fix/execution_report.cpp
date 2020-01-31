/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/execution_report.h"

#include "roq/core/charconv.h"

#include "roq/core/fix/array.h"
#include "roq/core/fix/exception.h"
#include "roq/core/fix/execution_report.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::ExecutionReport::has_field(field);
}
}  // namespace

ExecutionReport ExecutionReport::parse(
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  ExecutionReport result;
  parse(result, message, buffer);
  return result;
}

void ExecutionReport::parse(
    ExecutionReport& result,
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end(), buffer);
}

void ExecutionReport::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end,
    core::fix::Buffer& buffer) {
  while (iter != end) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::AVG_PX:
          static_assert(has_field(core::fix::Field::AVG_PX));
          core::fix::update(avg_px, value);
          break;
        case core::fix::Field::CL_ORD_ID:
          static_assert(has_field(core::fix::Field::CL_ORD_ID));
          core::fix::update(cl_ord_id, value);
          break;
        case core::fix::Field::COMMISSION:
          static_assert(has_field(core::fix::Field::COMMISSION));
          core::fix::update(commission, value);
          break;
        case core::fix::Field::CONTRACT_MULTIPLIER:
          static_assert(has_field(core::fix::Field::CONTRACT_MULTIPLIER));
          core::fix::update(contract_multiplier, value);
          break;
        case core::fix::Field::CUM_QTY:
          static_assert(has_field(core::fix::Field::CUM_QTY));
          core::fix::update(cum_qty, value);
          break;
        case core::fix::Field::EXEC_INST:
          static_assert(has_field(core::fix::Field::EXEC_INST));
          core::fix::update(exec_inst, value);
          break;
        case core::fix::Field::EXEC_TYPE:
          static_assert(has_field(core::fix::Field::EXEC_TYPE));
          core::fix::update(exec_type, value);
          break;
        case core::fix::Field::LAST_PX:
          static_assert(has_field(core::fix::Field::LAST_PX));
          core::fix::update(last_px, value);
          break;
        case core::fix::Field::LAST_QTY:
          static_assert(has_field(core::fix::Field::LAST_QTY));
          core::fix::update(last_qty, value);
          break;
        case core::fix::Field::LEAVES_QTY:
          static_assert(has_field(core::fix::Field::LEAVES_QTY));
          core::fix::update(leaves_qty, value);
          break;
        case core::fix::Field::MASS_STATUS_REQ_ID:
          static_assert(has_field(core::fix::Field::MASS_STATUS_REQ_ID));
          core::fix::update(mass_status_req_id, value);
          break;
        case core::fix::Field::MAX_SHOW:
          static_assert(has_field(core::fix::Field::MAX_SHOW));
          core::fix::update(max_show, value);
          break;
        case core::fix::Field::NO_FILLS: {
          static_assert(has_field(core::fix::Field::NO_FILLS));
          fills_grp = core::fix::Array<decltype(fills_grp)>::parse(
              buffer,
              iter,
              end);
          continue;  // note!
        }
        case core::fix::Field::ORD_REJ_REASON:
          static_assert(has_field(core::fix::Field::ORD_REJ_REASON));
          core::fix::update(ord_rej_reason, value);
          break;
        case core::fix::Field::ORD_STATUS:
          static_assert(has_field(core::fix::Field::ORD_STATUS));
          core::fix::update(ord_status, value);
          break;
        case core::fix::Field::ORD_TYPE:
          static_assert(has_field(core::fix::Field::ORD_TYPE));
          core::fix::update(ord_type, value);
          break;
        case core::fix::Field::ORDER_ID:
          static_assert(has_field(core::fix::Field::ORDER_ID));
          core::fix::update(order_id, value);
          break;
        case core::fix::Field::ORDER_QTY:
          static_assert(has_field(core::fix::Field::ORDER_QTY));
          core::fix::update(order_qty, value);
          break;
        case core::fix::Field::ORIG_CL_ORD_ID:
          static_assert(has_field(core::fix::Field::ORIG_CL_ORD_ID));
          core::fix::update(orig_cl_ord_id, value);
          break;
        case core::fix::Field::PEGGED_PRICE:
          static_assert(has_field(core::fix::Field::PEGGED_PRICE));
          core::fix::update(pegged_price, value);
          break;
        case core::fix::Field::PRICE:
          static_assert(has_field(core::fix::Field::PRICE));
          core::fix::update(price, value);
          break;
        case core::fix::Field::QTY_TYPE:
          static_assert(has_field(core::fix::Field::QTY_TYPE));
          core::fix::update(qty_type, value);
          break;
        case core::fix::Field::SECURITY_EXCHANGE:
          static_assert(has_field(core::fix::Field::SECURITY_EXCHANGE));
          core::fix::update(security_exchange, value);
          break;
        case core::fix::Field::SIDE:
          static_assert(has_field(core::fix::Field::SIDE));
          core::fix::update(side, value);
          break;
        case core::fix::Field::STOP_PX:
          static_assert(has_field(core::fix::Field::STOP_PX));
          core::fix::update(stop_px, value);
          break;
        case core::fix::Field::SYMBOL:
          static_assert(has_field(core::fix::Field::SYMBOL));
          core::fix::update(symbol, value);
          break;
        case core::fix::Field::TEXT:
          static_assert(has_field(core::fix::Field::TEXT));
          core::fix::update(text, value);
          break;
        case core::fix::Field::TOT_NUM_REPORTS:
          static_assert(has_field(core::fix::Field::TOT_NUM_REPORTS));
          core::fix::update(tot_num_reports, value);
          break;
        case core::fix::Field::TRANSACT_TIME:
          static_assert(has_field(core::fix::Field::TRANSACT_TIME));
          core::fix::update(transact_time, value);
          break;
        case core::fix::Field::VOLATILITY:
          static_assert(has_field(core::fix::Field::VOLATILITY));
          core::fix::update(volatility, value);
          break;
        // non-standard
        case core::fix::Field::MASS_STATUS_REQ_TYPE:
          static_assert(!has_field(core::fix::Field::MASS_STATUS_REQ_TYPE));
          core::fix::update(mass_status_req_type, value);
          break;
        default:
          if (has_field(field)) {
            DLOG(FATAL)("Unexpected tag={} field={}", tag, field);
            break;
          }
          switch (static_cast<Deribit>(tag)) {
            case Deribit::ADV_ORDER_TYPE:
              update(deribit_adv_order_type, value);
              break;
            case Deribit::LABEL:
              core::fix::update(deribit_label, value);
              break;
            default:
              DLOG(FATAL)("Unknown tag={} field={}", tag, field);
              throw core::fix::InvalidField(tag, value);
          }
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(tag, value, e);
    }
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
