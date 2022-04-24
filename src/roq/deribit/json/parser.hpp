/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_2.hpp"

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
    virtual void operator()(const Trace2<PlatformState const> &) = 0;
    virtual void operator()(const Trace2<InstrumentState const> &) = 0;
    virtual void operator()(const Trace2<Quote const> &) = 0;
    virtual void operator()(const Trace2<Ticker const> &) = 0;
    // private
    virtual void operator()(const Trace2<Portfolio const> &) = 0;
    virtual void operator()(const Trace2<Changes const> &) = 0;
    virtual void operator()(const Trace2<Order const> &) = 0;
    virtual void operator()(const Trace2<Trades2 const> &) = 0;
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
