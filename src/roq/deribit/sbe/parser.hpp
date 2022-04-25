/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <cstdint>
#include <span>

// events
#include <deribit_multicast/Book.h>
#include <deribit_multicast/Instrument.h>
#include <deribit_multicast/Quote.h>
#include <deribit_multicast/Trades.h>
// snapshot
#include <deribit_multicast/Snapshot.h>

#include "roq/trace.hpp"

#include "roq/deribit/sbe/frame.hpp"

namespace roq {
namespace deribit {
namespace sbe {

struct Parser final {
  struct Handler {
    // events
    virtual void operator()(const Trace<deribit_multicast::Instrument> &, const Frame &) = 0;
    virtual void operator()(const Trace<deribit_multicast::Book> &, const Frame &) = 0;
    virtual void operator()(const Trace<deribit_multicast::Quote> &, const Frame &) = 0;
    virtual void operator()(const Trace<deribit_multicast::Trades> &, const Frame &) = 0;
    // snapshot
    virtual void operator()(const Trace<deribit_multicast::Snapshot> &, const Frame &) = 0;
  };

  static bool dispatch(Handler &, const std::span<std::byte const> &buffer, const TraceInfo &);
};

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
