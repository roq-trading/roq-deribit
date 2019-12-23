/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <limits>
#include <string_view>

#include "roq/core/json/parser.h"

#include "roq/deribit/api/enums.h"

namespace roq {
namespace deribit {
namespace json {

struct Instrument final {
  std::string_view base_currency;
  double contract_size = std::numeric_limits<double>::quiet_NaN();
  std::chrono::nanoseconds creation_timestamp = {};
  std::chrono::nanoseconds expiration_timestamp = {};
  std::string_view instrument_name;
  bool is_active = false;
  api::Kind kind = api::Kind::UNKNOWN;
  double maker_commission = std::numeric_limits<double>::quiet_NaN();
  double max_leverage = std::numeric_limits<double>::quiet_NaN();
  double min_trade_amount = std::numeric_limits<double>::quiet_NaN();
  api::OptionType option_type = api::OptionType::UNKNOWN;
  std::string_view quote_currency;
  std::string_view settlement_period;  // TODO(thraneh): enum?
  double strike = std::numeric_limits<double>::quiet_NaN();
  double taker_commission = std::numeric_limits<double>::quiet_NaN();
  double tick_size = std::numeric_limits<double>::quiet_NaN();

  static void parse(Instrument&, core::json::object_t&&);
};

}  // namespace json
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::json::Instrument> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::json::Instrument& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "base_currency=\"{}\", "
        "contract_size={}, "
        "creation_timestamp={}, "
        "expiration_timestamp={}, "
        "instrument_name=\"{}\", "
        "is_activate={}, "
        "kind={}, "
        "min_trade_amount={}, "
        "option_type={}, "
        "quote_currency=\"{}\", "
        "settlement_period=\"{}\", "
        "strike={}, "
        "tick_size={}"
        "}}",
        value.base_currency,
        value.contract_size,
        value.creation_timestamp,
        value.expiration_timestamp,
        value.instrument_name,
        value.is_active,
        value.kind,
        value.min_trade_amount,
        value.option_type,
        value.quote_currency,
        value.settlement_period,
        value.strike,
        value.tick_size);
  }
};
