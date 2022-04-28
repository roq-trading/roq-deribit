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
    virtual void operator()(const Trace<PlatformState const> &) = 0;
    virtual void operator()(const Trace<InstrumentState const> &) = 0;
    virtual void operator()(const Trace<Quote const> &) = 0;
    virtual void operator()(const Trace<Ticker const> &) = 0;
    // private
    virtual void operator()(const Trace<Portfolio const> &) = 0;
    virtual void operator()(const Trace<Changes const> &) = 0;
    virtual void operator()(const Trace<Order const> &) = 0;
    virtual void operator()(const Trace<Trades2 const> &) = 0;
  };

  static void dispatch(Handler &, core::json::Value &, core::json::Buffer &, const TraceInfo &);
};

}  // namespace json
}  // namespace deribit
}  // namespace roq
