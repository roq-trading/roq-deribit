/* Copyright (c) 2017-2020, Hans Erik Thrane */

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

struct PositionQty final {
  // standard
  core::fix::PosType pos_type = core::fix::PosType::UNKNOWN;
  double long_qty = std::numeric_limits<double>::quiet_NaN();
  double short_qty = std::numeric_limits<double>::quiet_NaN();
  // non-standard
  double contract_multiplier = std::numeric_limits<double>::quiet_NaN();
  core::fix::QtyType qty_type = core::fix::QtyType::UNKNOWN;
  std::string_view raw_data;  // TODO(thraneh): parse?
  double settl_price = std::numeric_limits<double>::quiet_NaN();
  core::fix::Side side = core::fix::Side::UNKNOWN;
  std::string_view symbol;
  double underlying_price = std::numeric_limits<double>::quiet_NaN();
  // deribit specific
  double deribit_liquidation_price = std::numeric_limits<double>::quiet_NaN();
  double deribit_size_in_currency = std::numeric_limits<double>::quiet_NaN();

  static PositionQty parse(const core::fix::message_t& message);
  static void parse(PositionQty&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator& iter,
      const core::fix::message_t::const_iterator& end);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::PositionQty> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::PositionQty& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "long_qty={}, "
        "short_qty={}, "
        // non-standard
        "contract_multiplier={}, "
        "qty_type={}, "
        "raw_data=\"{}\", "
        "settl_price={}, "
        "side={}, "
        "symbol=\"{}\", "
        "underlying_price={}, "
        // deribit specific
        "deribit_liquidation_price={}, "
        "deribit_size_in_currency={}"
        "}}",
        value.long_qty,
        value.short_qty,
        // non-standard
        value.contract_multiplier,
        value.qty_type,
        value.raw_data,
        value.settl_price,
        value.side,
        value.symbol,
        value.underlying_price,
        // deribit specific
        value.deribit_liquidation_price,
        value.deribit_size_in_currency);
  }
};
