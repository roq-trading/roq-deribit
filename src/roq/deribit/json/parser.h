/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/buffer.h"
#include "roq/core/json/parser.h"

#include "roq/server.h"

#include "roq/deribit/json/ticker.h"

namespace roq {
namespace deribit {
namespace json {

struct Parser final {
  struct Handler {
    virtual void operator()(const Ticker &, const server::TraceInfo &) = 0;
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
