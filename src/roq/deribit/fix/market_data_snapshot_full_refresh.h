/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <limits>
#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct MarketDataSnapshotFullRefresh final {
  double contract_multiplier = std::numeric_limits<double>::quiet_NaN();
  double mark_price = std::numeric_limits<double>::quiet_NaN();
  std::string_view md_req_id;
  double open_interest = std::numeric_limits<double>::quiet_NaN();
  std::string_view symbol;
  double trade_volume_24h = std::numeric_limits<double>::quiet_NaN();
  double underlying_px = std::numeric_limits<double>::quiet_NaN();
  std::string_view underlying_symbol;
  struct {
    struct {
      std::string_view deribit_label;
      uint64_t deribit_trade_id;
      std::chrono::nanoseconds md_entry_date;
      double md_entry_px = std::numeric_limits<double>::quiet_NaN();
      double md_entry_size = std::numeric_limits<double>::quiet_NaN();
      core::fix::MDEntryType md_entry_type;
      core::fix::MDUpdateAction md_update_action;
      core::fix::OrdStatus ord_status;
      std::string_view secondary_order_id;
      core::fix::Side side;
      std::string_view text;
    } *items = nullptr;
    size_t length = 0;
  } md_full_grp;  // MDFullGrp

  using MDFullGrp = std::remove_pointer<decltype(md_full_grp.items)>::type;

  static MarketDataSnapshotFullRefresh parse(const core::fix::message_t& message);
  static void parse(MarketDataSnapshotFullRefresh&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::MarketDataSnapshotFullRefresh::MDFullGrp> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::MarketDataSnapshotFullRefresh::MDFullGrp& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "deribit_label=\"{}\", "
        "deribit_trade_id={}, "
        "md_entry_date={}, "
        "md_entry_px={}, "
        "md_entry_size={}, "
        "md_entry_type={}, "
        "md_update_action={}, "
        "ord_status={}, "
        "secondary_order_id=\"{}\", "
        "side={}, "
        "text=\"{}\""
        "}}",
        value.deribit_label,
        value.deribit_trade_id,
        value.md_entry_date,
        value.md_entry_px,
        value.md_entry_size,
        value.md_entry_type,
        value.md_update_action,
        value.ord_status,
        value.secondary_order_id,
        value.side,
        value.text);
  }
};

template <>
struct fmt::formatter<roq::deribit::fix::MarketDataSnapshotFullRefresh> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::MarketDataSnapshotFullRefresh& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "contract_multiplier={}, "
        "mark_price={}, "
        "md_req_id=\"{}\", "
        "open_interest={}, "
        "symbol=\"{}\", "
        "trade_volume_24h={}, "
        "underlying_px={}, "
        "underlying_symbol=\"{}\", "
        "md_full_grp=[{}]"
        "}}",
        value.contract_multiplier,
        value.mark_price,
        value.md_req_id,
        value.open_interest,
        value.symbol,
        value.trade_volume_24h,
        value.underlying_px,
        value.underlying_symbol,
        fmt::join(
            value.md_full_grp.items,
            value.md_full_grp.items + value.md_full_grp.length,
            ", "));
  }
};
