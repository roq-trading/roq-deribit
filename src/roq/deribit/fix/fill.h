/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <limits>
#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct Fill final {
  std::string_view fill_exec_id;
  double fill_px = std::numeric_limits<double>::quiet_NaN();
  double fill_qty = std::numeric_limits<double>::quiet_NaN();
  core::fix::FillLiquidityInd fill_liquidity_ind = core::fix::FillLiquidityInd::UNKNOWN;

 public:
  Fill(
      core::fix::message_t::const_iterator& iter,
      const core::fix::message_t::const_iterator& end);

  Fill(Fill&&) = default;
  Fill(const Fill&) = delete;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::Fill> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::Fill& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "fill_exec_id=\"{}\", "
        "fill_px={}, "
        "fill_qty={}, "
        "fill_liquidity_ind={}"
        "}}",
        value.fill_exec_id,
        value.fill_px,
        value.fill_qty,
        value.fill_liquidity_ind);
  }
};
