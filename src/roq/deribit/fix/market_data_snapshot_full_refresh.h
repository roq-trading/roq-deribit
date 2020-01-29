/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <cstddef>
#include <limits>
#include <string_view>

#include "roq/core/fix/buffer.h"
#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/md_full.h"

namespace roq {
namespace deribit {
namespace fix {

struct MarketDataSnapshotFullRefresh final {
  // standard
  double contract_multiplier = std::numeric_limits<double>::quiet_NaN();
  roq::span<MDFull const> md_full_grp;
  std::string_view md_req_id;
  std::string_view symbol;
  // non-standard
  double open_interest = std::numeric_limits<double>::quiet_NaN();
  double underlying_px = std::numeric_limits<double>::quiet_NaN();
  std::string_view underlying_symbol;
  // deribit specific
  double deribit_mark_price = std::numeric_limits<double>::quiet_NaN();
  double deribit_trade_volume_24h = std::numeric_limits<double>::quiet_NaN();

  static MarketDataSnapshotFullRefresh parse(
      const core::fix::message_t& message,
      core::fix::Buffer& buffer);

  static void parse(
      MarketDataSnapshotFullRefresh&,
      const core::fix::message_t& message,
      core::fix::Buffer& buffer);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end,
      core::fix::Buffer& buffer);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::MarketDataSnapshotFullRefresh> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::MarketDataSnapshotFullRefresh& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "contract_multiplier={}, "
        "md_full_grp=[{}], "
        "md_req_id=\"{}\", "
        "symbol=\"{}\", "
        // non-standard
        "open_interest={}, "
        "underlying_px={}, "
        "underlying_symbol=\"{}\", "
        // deribit specific
        "deribit_mark_price={}, "
        "deribit_trade_volume_24h={}"
        "}}",
        value.contract_multiplier,
        fmt::join(value.md_full_grp, ", "),
        value.md_req_id,
        value.symbol,
        // non-standard
        value.open_interest,
        value.underlying_px,
        value.underlying_symbol,
        // deribit specific
        value.deribit_mark_price,
        value.deribit_trade_volume_24h);
  }
};
