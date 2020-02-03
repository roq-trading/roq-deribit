/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <cstddef>
#include <limits>
#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct MDInc final {
  std::chrono::nanoseconds md_entry_date = {};
  double md_entry_px = std::numeric_limits<double>::quiet_NaN();
  double md_entry_size = std::numeric_limits<double>::quiet_NaN();
  core::fix::MDEntryType md_entry_type = core::fix::MDEntryType::UNKNOWN;
  core::fix::MDUpdateAction md_update_action = core::fix::MDUpdateAction::UNKNOWN;  // key
  std::string_view order_id;
  std::string_view secondary_order_id;
  std::string_view text;
  // non-standard
  double index_price = std::numeric_limits<double>::quiet_NaN();
  core::fix::OrdStatus ord_status = core::fix::OrdStatus::UNKNOWN;
  core::fix::Side side = core::fix::Side::UNKNOWN;
  // deribit specific
  std::string_view deribit_label;
  std::string_view deribit_liquidation;
  std::string_view deribit_trade_id;

 public:
  MDInc(
      core::fix::message_t::const_iterator& iter,
      const core::fix::message_t::const_iterator& end);

  MDInc(MDInc&&) = default;
  MDInc(const MDInc&) = delete;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::MDInc> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::MDInc& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "md_entry_date={}, "
        "md_entry_px={}, "
        "md_entry_size={}, "
        "md_entry_type={}, "
        "md_update_action={}, "
        "order_id=\"{}\", "
        "secondary_order_id=\"{}\", "
        "text=\"{}\", "
        // non-standard
        "index_price={}, "
        "ord_status={}, "
        "side={}, "
        // deribit specific
        "deribit_label=\"{}\", "
        "deribit_liquidation=\"{}\", "
        "deribit_trade_id=\"{}\""
        "}}",
        value.md_entry_date,
        value.md_entry_px,
        value.md_entry_size,
        value.md_entry_type,
        value.md_update_action,
        value.order_id,
        value.secondary_order_id,
        value.text,
        // non-standard
        value.index_price,
        value.ord_status,
        value.side,
        // deribit specific
        value.deribit_label,
        value.deribit_liquidation,
        value.deribit_trade_id);
  }
};
