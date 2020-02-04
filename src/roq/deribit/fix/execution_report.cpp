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
constexpr bool has_field(const auto& field) {
  return core::fix::ExecutionReport::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

template <auto field>
constexpr void non_standard_field() {
  static_assert(has_field(field) == false);
}

void update(
    auto& result,
    auto&& iter,
    const auto& end,
    auto& buffer) {
  while (iter != end) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::AVG_PX:
          check_field<core::fix::Field::AVG_PX>();
          core::fix::update(result.avg_px, value);
          break;
        case core::fix::Field::CL_ORD_ID:
          check_field<core::fix::Field::CL_ORD_ID>();
          core::fix::update(result.cl_ord_id, value);
          break;
        case core::fix::Field::COMMISSION:
          check_field<core::fix::Field::COMMISSION>();
          core::fix::update(result.commission, value);
          break;
        case core::fix::Field::CONTRACT_MULTIPLIER:
          check_field<core::fix::Field::CONTRACT_MULTIPLIER>();
          core::fix::update(result.contract_multiplier, value);
          break;
        case core::fix::Field::CUM_QTY:
          check_field<core::fix::Field::CUM_QTY>();
          core::fix::update(result.cum_qty, value);
          break;
        case core::fix::Field::EXEC_INST:
          check_field<core::fix::Field::EXEC_INST>();
          core::fix::update(result.exec_inst, value);
          break;
        case core::fix::Field::EXEC_TYPE:
          check_field<core::fix::Field::EXEC_TYPE>();
          core::fix::update(result.exec_type, value);
          break;
        case core::fix::Field::LAST_PX:
          check_field<core::fix::Field::LAST_PX>();
          core::fix::update(result.last_px, value);
          break;
        case core::fix::Field::LAST_QTY:
          check_field<core::fix::Field::LAST_QTY>();
          core::fix::update(result.last_qty, value);
          break;
        case core::fix::Field::LEAVES_QTY:
          check_field<core::fix::Field::LEAVES_QTY>();
          core::fix::update(result.leaves_qty, value);
          break;
        case core::fix::Field::MASS_STATUS_REQ_ID:
          check_field<core::fix::Field::MASS_STATUS_REQ_ID>();
          core::fix::update(result.mass_status_req_id, value);
          break;
        case core::fix::Field::MAX_SHOW:
          check_field<core::fix::Field::MAX_SHOW>();
          core::fix::update(result.max_show, value);
          break;
        case core::fix::Field::NO_FILLS: {
          check_field<core::fix::Field::NO_FILLS>();
          result.fills_grp =
            core::fix::Array<decltype(result.fills_grp)>::create(
                buffer,
                iter,
                end);
          continue;  // note!
        }
        case core::fix::Field::ORD_REJ_REASON:
          check_field<core::fix::Field::ORD_REJ_REASON>();
          core::fix::update(result.ord_rej_reason, value);
          break;
        case core::fix::Field::ORD_STATUS:
          check_field<core::fix::Field::ORD_STATUS>();
          core::fix::update(result.ord_status, value);
          break;
        case core::fix::Field::ORD_TYPE:
          check_field<core::fix::Field::ORD_TYPE>();
          core::fix::update(result.ord_type, value);
          break;
        case core::fix::Field::ORDER_ID:
          check_field<core::fix::Field::ORDER_ID>();
          core::fix::update(result.order_id, value);
          break;
        case core::fix::Field::ORDER_QTY:
          check_field<core::fix::Field::ORDER_QTY>();
          core::fix::update(result.order_qty, value);
          break;
        case core::fix::Field::ORIG_CL_ORD_ID:
          check_field<core::fix::Field::ORIG_CL_ORD_ID>();
          core::fix::update(result.orig_cl_ord_id, value);
          break;
        case core::fix::Field::PEGGED_PRICE:
          check_field<core::fix::Field::PEGGED_PRICE>();
          core::fix::update(result.pegged_price, value);
          break;
        case core::fix::Field::PRICE:
          check_field<core::fix::Field::PRICE>();
          core::fix::update(result.price, value);
          break;
        case core::fix::Field::QTY_TYPE:
          check_field<core::fix::Field::QTY_TYPE>();
          core::fix::update(result.qty_type, value);
          break;
        case core::fix::Field::SECURITY_EXCHANGE:
          check_field<core::fix::Field::SECURITY_EXCHANGE>();
          core::fix::update(result.security_exchange, value);
          break;
        case core::fix::Field::SIDE:
          check_field<core::fix::Field::SIDE>();
          core::fix::update(result.side, value);
          break;
        case core::fix::Field::STOP_PX:
          check_field<core::fix::Field::STOP_PX>();
          core::fix::update(result.stop_px, value);
          break;
        case core::fix::Field::SYMBOL:
          check_field<core::fix::Field::SYMBOL>();
          core::fix::update(result.symbol, value);
          break;
        case core::fix::Field::TEXT:
          check_field<core::fix::Field::TEXT>();
          core::fix::update(result.text, value);
          break;
        case core::fix::Field::TOT_NUM_REPORTS:
          check_field<core::fix::Field::TOT_NUM_REPORTS>();
          core::fix::update(result.tot_num_reports, value);
          break;
        case core::fix::Field::TRANSACT_TIME:
          check_field<core::fix::Field::TRANSACT_TIME>();
          core::fix::update(result.transact_time, value);
          break;
        case core::fix::Field::VOLATILITY:
          check_field<core::fix::Field::VOLATILITY>();
          core::fix::update(result.volatility, value);
          break;
        // non-standard
        case core::fix::Field::MASS_STATUS_REQ_TYPE:
          non_standard_field<core::fix::Field::MASS_STATUS_REQ_TYPE>();
          core::fix::update(result.mass_status_req_type, value);
          break;
        default:
          if (has_field(field)) {
            DLOG(FATAL)(
                FMT_STRING("Unexpected tag={} field={}"),
                tag,
                field);
            break;
          }
          switch (static_cast<Deribit>(tag)) {
            case Deribit::ADV_ORDER_TYPE:
              update(result.deribit_adv_order_type, value);
              break;
            case Deribit::LABEL:
              core::fix::update(result.deribit_label, value);
              break;
            default:
              DLOG(FATAL)(
                  FMT_STRING("Unknown tag={} field={}"),
                  tag,
                  field);
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
}  // namespace

ExecutionReport ExecutionReport::create(
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  ExecutionReport result;
  update(
      result,
      message.begin(),
      message.end(),
      buffer);
  return result;
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
