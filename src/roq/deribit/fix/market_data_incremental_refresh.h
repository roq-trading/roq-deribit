/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <cstddef>
#include <limits>
#include <string_view>
#include <vector>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct MarketDataIncrementalRefresh final {
  double contract_multiplier = std::numeric_limits<double>::quiet_NaN();
  double mark_price = std::numeric_limits<double>::quiet_NaN();
  std::string_view md_req_id;
  double open_interest = std::numeric_limits<double>::quiet_NaN();
  std::string_view symbol;
  double trade_volume_24h = std::numeric_limits<double>::quiet_NaN();

  struct {
    struct {
      std::string_view deribit_label;
      uint64_t deribit_trade_id;
      double index_price = std::numeric_limits<double>::quiet_NaN();
      std::chrono::nanoseconds md_entry_date;
      double md_entry_px = std::numeric_limits<double>::quiet_NaN();
      double md_entry_size = std::numeric_limits<double>::quiet_NaN();
      core::fix::MDEntryType md_entry_type;
      core::fix::MDUpdateAction md_update_action;
      core::fix::OrdStatus ord_status;
      std::string_view order_id;
      std::string_view secondary_order_id;
      core::fix::Side side;
    } *items = nullptr;
    size_t length = 0;
  } md_inc_grp;  // MDIncGrp

  using MDIncGrp = std::remove_pointer<decltype(md_inc_grp.items)>::type;

  static MarketDataIncrementalRefresh parse(
      const core::fix::message_t& message,
      std::vector<std::byte>& buffer);

  static void parse(
      MarketDataIncrementalRefresh&,
      const core::fix::message_t& message,
      std::vector<std::byte>& buffer);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end,
      std::vector<std::byte>& buffer);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::MarketDataIncrementalRefresh::MDIncGrp> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::MarketDataIncrementalRefresh::MDIncGrp& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "deribit_label=\"{}\", "
        "deribit_trade_id={}, "
        "index_price={}, "
        "md_entry_date={}, "
        "md_entry_px={}, "
        "md_entry_size={}, "
        "md_entry_type={}, "
        "md_update_action={}, "
        "ord_status={}, "
        "order_id=\"{}\", "
        "secondary_order_id=\"{}\", "
        "side={}"
        "}}",
        value.deribit_label,
        value.deribit_trade_id,
        value.index_price,
        value.md_entry_date,
        value.md_entry_px,
        value.md_entry_size,
        value.md_entry_type,
        value.md_update_action,
        value.ord_status,
        value.order_id,
        value.secondary_order_id,
        value.side);
  }
};

template <>
struct fmt::formatter<roq::deribit::fix::MarketDataIncrementalRefresh> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::MarketDataIncrementalRefresh& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "contract_multiplier={}, "
        "mark_price={}, "
        "md_req_id=\"{}\", "
        "open_interest={}, "
        "symbol=\"{}\", "
        "trade_volume_24h={}, "
        "md_inc_grp=[{}]"
        "}}",
        value.contract_multiplier,
        value.mark_price,
        value.md_req_id,
        value.open_interest,
        value.symbol,
        value.trade_volume_24h,
        fmt::join(
            value.md_inc_grp.items,
            value.md_inc_grp.items + value.md_inc_grp.length,
            ", "));
  }
};
