/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

// public
#include "roq/deribit/json/ticker.hpp"

// private
#include "roq/deribit/json/changes.hpp"
#include "roq/deribit/json/instrument_state.hpp"
#include "roq/deribit/json/order.hpp"
#include "roq/deribit/json/platform_state.hpp"
#include "roq/deribit/json/portfolio.hpp"
#include "roq/deribit/json/quote.hpp"
#include "roq/deribit/json/trades_2.hpp"

namespace roq {
namespace deribit {
namespace json {

struct Parser final {
  struct Handler {
    // public
    virtual void operator()(Trace<PlatformState> const &) = 0;
    virtual void operator()(Trace<InstrumentState> const &) = 0;
    virtual void operator()(Trace<Quote> const &) = 0;
    virtual void operator()(Trace<Ticker> const &) = 0;
    // private
    virtual void operator()(Trace<Portfolio> const &) = 0;
    virtual void operator()(Trace<Changes> const &) = 0;
    virtual void operator()(Trace<Order> const &) = 0;
    virtual void operator()(Trace<Trades2> const &) = 0;
  };

  static bool dispatch(Handler &, core::json::Value &, std::span<std::byte> const &, TraceInfo const &);
};

}  // namespace json
}  // namespace deribit
}  // namespace roq
