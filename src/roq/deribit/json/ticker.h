/* Copyright (c) 2017-2019, Hans Erik Thrane */

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

struct Ticker final {
  double best_ask_amount = std::numeric_limits<double>::quiet_NaN();
  double best_ask_price = std::numeric_limits<double>::quiet_NaN();
  double best_bid_amount = std::numeric_limits<double>::quiet_NaN();
  double best_bid_price = std::numeric_limits<double>::quiet_NaN();
  double current_funding = std::numeric_limits<double>::quiet_NaN();
  double funding_8h = std::numeric_limits<double>::quiet_NaN();
  double index_price = std::numeric_limits<double>::quiet_NaN();
  std::string_view instrument_name;
  double last_price = std::numeric_limits<double>::quiet_NaN();
  double mark_price = std::numeric_limits<double>::quiet_NaN();
  double max_price = std::numeric_limits<double>::quiet_NaN();
  double min_price = std::numeric_limits<double>::quiet_NaN();
  double open_interest = std::numeric_limits<double>::quiet_NaN();
  double settlement_price = std::numeric_limits<double>::quiet_NaN();
  api::State state = api::State::UNKNOWN;
  std::chrono::nanoseconds timestamp = {};
  // stats:
  double high = std::numeric_limits<double>::quiet_NaN();
  double low = std::numeric_limits<double>::quiet_NaN();
  double volume = std::numeric_limits<double>::quiet_NaN();

  static void parse(Ticker&, core::json::object_t&&);
};

}  // namespace json
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::json::Ticker> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::json::Ticker& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "best_ask_amount={}, "
        "best_ask_price={}, "
        "best_bid_amount={}, "
        "best_bid_price={}, "
        "current_funding={}, "
        "funding_8h={}, "
        "instrument_name=\"{}\", "
        "last_price={}, "
        "mark_price={}, "
        "max_price={}, "
        "min_price={}, "
        "open_interest={}, "
        "settlement_price={}, "
        "state={}, "
        "timestamp={}, "
        "high={}, "
        "low={}, "
        "volume={}"
        "}}",
        value.best_ask_amount,
        value.best_ask_price,
        value.best_bid_amount,
        value.best_bid_price,
        value.current_funding,
        value.funding_8h,
        value.instrument_name,
        value.last_price,
        value.mark_price,
        value.max_price,
        value.min_price,
        value.open_interest,
        value.settlement_price,
        value.state,
        value.timestamp,
        value.high,
        value.low,
        value.volume);
  }
};
