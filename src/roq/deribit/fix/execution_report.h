/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <limits>
#include <string_view>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct ExecutionReport final {
  // standard?
  double avg_px = std::numeric_limits<double>::quiet_NaN();
  std::string_view cl_ord_id;
double contract_multiplier = std::numeric_limits<double>::quiet_NaN();
  double cum_qty = std::numeric_limits<double>::quiet_NaN();
  std::string_view exec_inst;  // TODO(thraneh): MultipleCharValue
  core::fix::ExecType exec_type = core::fix::ExecType::UNKNOWN;
  double leaves_qty = std::numeric_limits<double>::quiet_NaN();
  std::string_view mass_status_req_id;
  double max_show = std::numeric_limits<double>::quiet_NaN();
  core::fix::OrdRejReason ord_rej_reason;
  core::fix::OrdStatus ord_status = core::fix::OrdStatus::UNKNOWN;
  core::fix::OrdType ord_type = core::fix::OrdType::UNKNOWN;
  std::string_view order_id;
  double order_qty = std::numeric_limits<double>::quiet_NaN();
  std::string_view orig_cl_ord_id;
  bool pegged_price = false;
  double price = std::numeric_limits<double>::quiet_NaN();
  core::fix::QtyType qty_type = core::fix::QtyType::UNKNOWN;
  std::string_view security_exchange;
  core::fix::Side side = core::fix::Side::UNKNOWN;
  double stop_px = std::numeric_limits<double>::quiet_NaN();
  std::string_view symbol;
  std::string_view text;
  uint32_t tot_num_reports = 0;
  std::chrono::nanoseconds transact_time;
  double volatility = std::numeric_limits<double>::quiet_NaN();
  // non-standard
  core::fix::MassStatusReqType mass_status_req_type = core::fix::MassStatusReqType::UNKNOWN;
  // deribit specific
  AdvOrderType deribit_adv_order_type = AdvOrderType::UNKNOWN;
  std::string_view deribit_label;

  static ExecutionReport parse(const core::fix::message_t& message);
  static void parse(ExecutionReport&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::ExecutionReport> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::ExecutionReport& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "avg_px={}, "
        "cl_ord_id=\"{}\", "
        "contract_multiplier={}, "
        "cum_qty={}, "
        "exec_inst=\"{}\", "
        "exec_type={}, "
        "leaves_qty={}, "
        "mass_status_req_id=\"{}\", "
        "max_show={}, "
        "ord_rej_reason={}, "
        "ord_status={}, "
        "ord_type={}, "
        "order_id=\"{}\", "
        "order_qty={}, "
        "orig_cl_ord_id=\"{}\", "
        "pegged_price={}, "
        "price={}, "
        "qty_type={}, "
        "security_exchange=\"{}\", "
        "side={}, "
        "stop_px={}, "
        "symbol=\"{}\", "
        "text=\"{}\", "
        "tot_num_reports={}, "
        "transact_time={}, "
        "volatility={}, "
        // non-standard
        "mass_status_req_type={}, "
        // deribit specific
        "deribit_adv_order_type={}, "
        "deribit_label=\"{}\""
        "}}",
        value.avg_px,
        value.cl_ord_id,
        value.contract_multiplier,
        value.cum_qty,
        value.exec_inst,
        value.exec_type,
        value.leaves_qty,
        value.mass_status_req_id,
        value.max_show,
        value.ord_rej_reason,
        value.ord_status,
        value.ord_type,
        value.order_id,
        value.order_qty,
        value.orig_cl_ord_id,
        value.pegged_price,
        value.price,
        value.qty_type,
        value.security_exchange,
        value.side,
        value.stop_px,
        value.symbol,
        value.text,
        value.tot_num_reports,
        value.transact_time,
        value.volatility,
        // non-standard
        value.mass_status_req_type,
        // deribit specific
        value.deribit_adv_order_type,
        value.deribit_label);
  }
};
