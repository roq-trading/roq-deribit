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

struct Instrument final {
  Instrument(
      core::fix::message_t::const_iterator& iter,
      const core::fix::message_t::const_iterator& end);

  Instrument(const Instrument&) = delete;
  Instrument(Instrument&&) = delete;

  double contract_multiplier = std::numeric_limits<double>::quiet_NaN();
  std::chrono::nanoseconds issue_date = {};
  std::chrono::nanoseconds maturity_date = {};
  std::chrono::nanoseconds maturity_time = {};
  double min_price_increment = std::numeric_limits<double>::quiet_NaN();
  core::fix::PutOrCall put_or_call = core::fix::PutOrCall::UNKNOWN;
  std::string_view security_desc;
  std::string_view security_type;
  std::string_view strike_currency;
  double strike_price = std::numeric_limits<double>::quiet_NaN();
  std::string_view symbol;
  // non-standard
  std::string_view comm_currency;
  std::string_view currency;
  double min_trade_vol = std::numeric_limits<double>::quiet_NaN();
  std::string_view settl_currency;
  core::fix::SettlType settl_type = core::fix::SettlType::UNKNOWN;
  std::string_view underlying_symbol;
  // deribit specific
  uint8_t deribit_instrument_price_precision = 0;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::Instrument> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::Instrument& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "contract_multiplier={}, "
        "issue_date={}, "
        "maturity_date={}, "
        "maturity_time={}, "
        "min_price_increment={}, "
        "put_or_call={}, "
        "security_desc=\"{}\", "
        "security_type=\"{}\", "
        "strike_currency=\"{}\", "
        "strike_price={}, "
        "symbol=\"{}\", "
        // non-standard
        "comm_currency=\"{}\", "
        "currency=\"{}\", "
        "min_trade_vol={}, "
        "settl_currency=\"{}\", "
        "settl_type={}, "
        "underlying_symbol=\"{}\", "
        // deribit specific
        "deribit_instrument_price_precision={}"
        "}}",
        value.contract_multiplier,
        value.issue_date,
        value.maturity_date,
        value.maturity_time,
        value.min_price_increment,
        value.put_or_call,
        value.security_desc,
        value.security_type,
        value.strike_currency,
        value.strike_price,
        value.symbol,
        // non-standard
        value.comm_currency,
        value.currency,
        value.min_trade_vol,
        value.settl_currency,
        value.settl_type,
        value.underlying_symbol,
        // deribit specific
        value.deribit_instrument_price_precision);
  }
};
