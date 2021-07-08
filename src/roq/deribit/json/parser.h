/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/buffer.h"
#include "roq/core/json/parser.h"

#include "roq/server.h"

// public
#include "roq/deribit/json/ticker.h"

// private
#include "roq/deribit/json/changes.h"
#include "roq/deribit/json/instrument_state.h"
#include "roq/deribit/json/order.h"
#include "roq/deribit/json/platform_state.h"
#include "roq/deribit/json/portfolio.h"
#include "roq/deribit/json/quote.h"

namespace roq {
namespace deribit {
namespace json {

struct Parser final {
  struct Handler {
    // public
    virtual void operator()(const server::Trace<PlatformState> &) = 0;
    virtual void operator()(const server::Trace<InstrumentState> &) = 0;
    virtual void operator()(const server::Trace<Quote> &) = 0;
    virtual void operator()(const server::Trace<Ticker> &) = 0;
    // private
    virtual void operator()(const server::Trace<Portfolio> &) = 0;
    virtual void operator()(const server::Trace<Changes> &) = 0;
    virtual void operator()(const server::Trace<Order> &) = 0;
  };

  static void dispatch(
      Handler &handler,
      core::json::value_t &value,
      core::json::Buffer &buffer,
      const server::TraceInfo &trace_info);
};

}  // namespace json
}  // namespace deribit
}  // namespace roq
