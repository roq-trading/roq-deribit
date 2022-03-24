/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/buffer.hpp"
#include "roq/core/json/parser.hpp"

#include "roq/server.hpp"

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
    virtual void operator()(const Trace<PlatformState> &) = 0;
    virtual void operator()(const Trace<InstrumentState> &) = 0;
    virtual void operator()(const Trace<Quote> &) = 0;
    virtual void operator()(const Trace<Ticker> &) = 0;
    // private
    virtual void operator()(const Trace<Portfolio> &) = 0;
    virtual void operator()(const Trace<Changes> &) = 0;
    virtual void operator()(const Trace<Order> &) = 0;
    virtual void operator()(const Trace<Trades2> &) = 0;
  };

  static void dispatch(
      Handler &handler,
      core::json::value_t &value,
      core::json::Buffer &buffer,
      const TraceInfo &trace_info);
};

}  // namespace json
}  // namespace deribit
}  // namespace roq
