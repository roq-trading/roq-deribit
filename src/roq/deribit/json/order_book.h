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

struct OrderBook final {
  double ask_iv = std::numeric_limits<double>::quiet_NaN();
  // asks
  double best_ask_amount = std::numeric_limits<double>::quiet_NaN();
  double best_ask_price = std::numeric_limits<double>::quiet_NaN();
  double best_bid_amount = std::numeric_limits<double>::quiet_NaN();
  double best_bid_price = std::numeric_limits<double>::quiet_NaN();
  double bid_iv = std::numeric_limits<double>::quiet_NaN();
  // bids
  double current_funding = std::numeric_limits<double>::quiet_NaN();
  double delivery_price = std::numeric_limits<double>::quiet_NaN();
  double funding_8h = std::numeric_limits<double>::quiet_NaN();
  // TODO(thraneh): greeks
  double index_price = std::numeric_limits<double>::quiet_NaN();
  std::string_view instrument_name;
  double interest_rate = std::numeric_limits<double>::quiet_NaN();
  double last_price = std::numeric_limits<double>::quiet_NaN();
  double mark_iv = std::numeric_limits<double>::quiet_NaN();
  double mark_price = std::numeric_limits<double>::quiet_NaN();
  double max_price = std::numeric_limits<double>::quiet_NaN();
  double min_price = std::numeric_limits<double>::quiet_NaN();
  double open_interest = std::numeric_limits<double>::quiet_NaN();
  double settlement_price = std::numeric_limits<double>::quiet_NaN();
  api::State state = api::State::UNKNOWN;
  // TODO(thraneh): stats
  std::chrono::nanoseconds timestamp;
  double underlying_index = std::numeric_limits<double>::quiet_NaN();
  double underlying_price = std::numeric_limits<double>::quiet_NaN();

  static void parse(OrderBook&, core::json::object_t&&);
};

}  // namespace json
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::json::OrderBook> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::json::OrderBook& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "ask_iv={}, "
        // asks
        "best_ask_amount={}, "
        "best_ask_price={}, "
        "best_bid_amount={}, "
        "best_bid_price={}, "
        "bid_iv={}, "
        // bids
        "current_funding={}, "
        "delivery_price={}, "
        "funding_8h={}, "
        // greeks
        "index_price={}, "
        "instrument_name=\"{}\", "
        "interest_rate={}, "
        "last_price={}, "
        "mark_iv={}, "
        "mark_price={}, "
        "max_price={}, "
        "min_price={}, "
        "open_interest={}, "
        "settlement_price={}, "
        "state={}, "
        // stats
        "timestamp={}, "
        "underlying_index={}, "
        "underlying_price={}"
        "}}",
        value.ask_iv,
        // asks
        value.best_ask_amount,
        value.best_ask_price,
        value.best_bid_amount,
        value.best_bid_price,
        value.bid_iv,
        // bids
        value.current_funding,
        value.delivery_price,
        value.funding_8h,
        // greeks
        value.index_price,
        value.instrument_name,
        value.interest_rate,
        value.last_price,
        value.mark_iv,
        value.mark_price,
        value.max_price,
        value.min_price,
        value.open_interest,
        value.settlement_price,
        value.state,
        // stats
        value.timestamp,
        value.underlying_index,
        value.underlying_price);
  }
};
