/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/execution_report.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/execution_report.h"
#include "roq/core/fix/fills.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/array.h"
#include "roq/deribit/fix/buffer.h"
#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
bool update_fills(
    auto& result,
    const auto& tag,
    const auto& field,
    const auto& value) {
  try {
    switch (field) {
      // key
      case core::fix::Field::FILL_EXEC_ID:
        static_assert(core::fix::FillsGrp::has_field(core::fix::Field::FILL_EXEC_ID));
        return false;  // break
      // standard
      case core::fix::Field::FILL_PX:
        static_assert(core::fix::FillsGrp::has_field(core::fix::Field::FILL_PX));
        core::fix::update(result.fill_px, value);
        break;
      case core::fix::Field::FILL_QTY:
        static_assert(core::fix::FillsGrp::has_field(core::fix::Field::FILL_QTY));
        core::fix::update(result.fill_qty, value);
        break;
      case core::fix::Field::FILL_LIQUIDITY_IND:
        static_assert(core::fix::FillsGrp::has_field(core::fix::Field::FILL_LIQUIDITY_IND));
        core::fix::update(result.fill_liquidity_ind, value);
        break;
      default:
        if (core::fix::Fills::has_field(field))
          break;
        return false;
    }
    return true;
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(
        "MarketDataIncrementalRefresh|MDIncGrp: "
        "Parse error: "
        "field={}, value=\"{}\", what=\"{}\"",
        tag, value, e.what());
  }
}

void parse_fills_grp(
    ExecutionReport::FillsGrp& result,
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  assert(iter != end);
  new (&result) std::remove_reference<decltype(result)>::type {};
  // key
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  static_assert(core::fix::Fills::key_field == core::fix::Field::FILL_EXEC_ID);
  if (field != core::fix::Fills::key_field)
    throw core::fix::InvalidField(
        "ExecutionReport|FillsGrp: "
        "Unexpected first field={}", tag);
  core::fix::update(result.fill_exec_id, value);
  // remaining fields
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    if (update_fills(result, tag, field, value) == false)
      return;
  }
}
}  // namespace


ExecutionReport ExecutionReport::parse(
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  ExecutionReport result;
  parse(result, message, buffer);
  return result;
}

void ExecutionReport::parse(
    ExecutionReport& result,
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end(), buffer);
}

void ExecutionReport::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end,
    std::vector<std::byte>& buffer) {
  Buffer buffer_(buffer);
  while (iter != end) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::AVG_PX:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::AVG_PX));
          core::fix::update(avg_px, value);
          break;
        case core::fix::Field::CL_ORD_ID:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::CL_ORD_ID));
          core::fix::update(cl_ord_id, value);
          break;
        case core::fix::Field::COMMISSION:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::COMMISSION));
          core::fix::update(commission, value);
          break;
        case core::fix::Field::CONTRACT_MULTIPLIER:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::CONTRACT_MULTIPLIER));
          core::fix::update(contract_multiplier, value);
          break;
        case core::fix::Field::CUM_QTY:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::CUM_QTY));
          core::fix::update(cum_qty, value);
          break;
        case core::fix::Field::EXEC_INST:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::EXEC_INST));
          core::fix::update(exec_inst, value);
          break;
        case core::fix::Field::EXEC_TYPE:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::EXEC_TYPE));
          core::fix::update(exec_type, value);
          break;
        case core::fix::Field::LAST_PX:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::LAST_PX));
          core::fix::update(last_px, value);
          break;
        case core::fix::Field::LAST_QTY:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::LAST_QTY));
          core::fix::update(last_qty, value);
          break;
        case core::fix::Field::LEAVES_QTY:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::LEAVES_QTY));
          core::fix::update(leaves_qty, value);
          break;
        case core::fix::Field::MASS_STATUS_REQ_ID:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::MASS_STATUS_REQ_ID));
          core::fix::update(mass_status_req_id, value);
          break;
        case core::fix::Field::MAX_SHOW:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::MAX_SHOW));
          core::fix::update(max_show, value);
          break;
        case core::fix::Field::NO_FILLS: {
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::NO_FILLS));
          auto length = core::charconv::from_string<uint32_t>(value);
          ++iter;
          Array array(buffer_, fills_grp);
          for (uint32_t i = 0; i < length; ++i) {
            if (iter == end)
              throw core::fix::UnexpectedEndOfMessage(
                  "ExecutionReport|FillsGrp");
            auto& item = array.next();
            parse_fills_grp(item, iter, end);
            ++array;
          }
          if (fills_grp.length != length)
            throw core::fix::InvalidGroupLength(
                "ExecutionReport|FillsGrp: "
                "Invalid group length: parsed={}, expected={}",
                fills_grp.length, length);
          continue;
        }
        case core::fix::Field::ORD_REJ_REASON:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::ORD_REJ_REASON));
          core::fix::update(ord_rej_reason, value);
          break;
        case core::fix::Field::ORD_STATUS:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::ORD_STATUS));
          core::fix::update(ord_status, value);
          break;
        case core::fix::Field::ORD_TYPE:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::ORD_TYPE));
          core::fix::update(ord_type, value);
          break;
        case core::fix::Field::ORDER_ID:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::ORDER_ID));
          core::fix::update(order_id, value);
          break;
        case core::fix::Field::ORDER_QTY:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::ORDER_QTY));
          core::fix::update(order_qty, value);
          break;
        case core::fix::Field::ORIG_CL_ORD_ID:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::ORIG_CL_ORD_ID));
          core::fix::update(orig_cl_ord_id, value);
          break;
        case core::fix::Field::PEGGED_PRICE:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::PEGGED_PRICE));
          core::fix::update(pegged_price, value);
          break;
        case core::fix::Field::PRICE:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::PRICE));
          core::fix::update(price, value);
          break;
        case core::fix::Field::QTY_TYPE:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::QTY_TYPE));
          core::fix::update(qty_type, value);
          break;
        case core::fix::Field::SECURITY_EXCHANGE:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::SECURITY_EXCHANGE));
          core::fix::update(security_exchange, value);
          break;
        case core::fix::Field::SIDE:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::SIDE));
          core::fix::update(side, value);
          break;
        case core::fix::Field::STOP_PX:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::STOP_PX));
          core::fix::update(stop_px, value);
          break;
        case core::fix::Field::SYMBOL:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::SYMBOL));
          core::fix::update(symbol, value);
          break;
        case core::fix::Field::TEXT:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::TEXT));
          core::fix::update(text, value);
          break;
        case core::fix::Field::TOT_NUM_REPORTS:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::TOT_NUM_REPORTS));
          core::fix::update(tot_num_reports, value);
          break;
        case core::fix::Field::TRANSACT_TIME:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::TRANSACT_TIME));
          core::fix::update(transact_time, value);
          break;
        case core::fix::Field::VOLATILITY:
          static_assert(core::fix::ExecutionReport::has_field(core::fix::Field::VOLATILITY));
          core::fix::update(volatility, value);
          break;
        // non-standard
        case core::fix::Field::MASS_STATUS_REQ_TYPE:
          static_assert(!core::fix::ExecutionReport::has_field(core::fix::Field::MASS_STATUS_REQ_TYPE));
          core::fix::update(mass_status_req_type, value);
          break;
        default:
          if (core::fix::ExecutionReport::has_field(field))
            break;
          switch (static_cast<Deribit>(tag)) {
            case Deribit::ADV_ORDER_TYPE:
              update(deribit_adv_order_type, value);
              break;
            case Deribit::LABEL:
              core::fix::update(deribit_label, value);
              break;
            default:
              throw core::fix::InvalidField(
                  "ExecutionReport: "
                  "Unexpected field={}", tag);
          }
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(
          "ExecutionReport: "
          "Parse error: "
          "field={}, value=\"{}\", what=\"{}\"",
          tag, value, e.what());
    }
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
