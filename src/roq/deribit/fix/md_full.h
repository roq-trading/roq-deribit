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

struct MDFull final {
  std::chrono::nanoseconds md_entry_date = {};
  double md_entry_px = std::numeric_limits<double>::quiet_NaN();
  double md_entry_size = std::numeric_limits<double>::quiet_NaN();
  core::fix::MDEntryType md_entry_type = core::fix::MDEntryType::UNKNOWN;  // key
  std::string_view secondary_order_id;
  std::string_view text;
  // non-standard
  core::fix::MDUpdateAction md_update_action = core::fix::MDUpdateAction::UNKNOWN;
  core::fix::OrdStatus ord_status = core::fix::OrdStatus::UNKNOWN;
  core::fix::Side side = core::fix::Side::UNKNOWN;
  // deribit specific
  std::string_view deribit_label;
  std::string_view deribit_liquidation;
  uint64_t deribit_trade_id = 0;

 public:
  MDFull(
      core::fix::message_t::const_iterator& iter,
      const core::fix::message_t::const_iterator& end);

  MDFull(MDFull&&) = default;
  MDFull(const MDFull&) = delete;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::MDFull> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::MDFull& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "md_entry_date={}, "
        "md_entry_px={}, "
        "md_entry_size={}, "
        "md_entry_type={}, "
        "secondary_order_id=\"{}\", "
        "text=\"{}\", "
        "md_update_action={}, "
        "ord_status={}, "
        "side={}, "
        "deribit_label=\"{}\", "
        "deribit_liquidation={}, "
        "deribit_trade_id={}"
        "}}",
        value.md_entry_date,
        value.md_entry_px,
        value.md_entry_size,
        value.md_entry_type,
        value.secondary_order_id,
        value.text,
        value.md_update_action,
        value.ord_status,
        value.side,
        value.deribit_label,
        value.deribit_liquidation,
        value.deribit_trade_id);
  }
};
