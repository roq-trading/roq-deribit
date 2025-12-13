/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <span>

#include <deribit_multicast/Book.h>           // 1001
#include <deribit_multicast/ComboLegs.h>      // 1007
#include <deribit_multicast/Instrument.h>     // 1000
#include <deribit_multicast/InstrumentV2.h>   // 1010
#include <deribit_multicast/PriceIndex.h>     // 1008
#include <deribit_multicast/Rfq.h>            // 1009
#include <deribit_multicast/Snapshot.h>       // 1004
#include <deribit_multicast/SnapshotEnd.h>    // 1006
#include <deribit_multicast/SnapshotStart.h>  // 1005
#include <deribit_multicast/Ticker.h>         // 1003
#include <deribit_multicast/Trades.h>         // 1002

#include "roq/trace.hpp"

#include "roq/deribit/sbe/frame.hpp"

namespace roq {
namespace deribit {
namespace sbe {

struct Parser final {
  struct Handler {
    virtual bool operator()(Frame const &) = 0;

    virtual void operator()(Trace<deribit_multicast::Instrument> const &, Frame const &) = 0;     // 1000
    virtual void operator()(Trace<deribit_multicast::Book> const &, Frame const &) = 0;           // 1001
    virtual void operator()(Trace<deribit_multicast::Trades> const &, Frame const &) = 0;         // 1002
    virtual void operator()(Trace<deribit_multicast::Ticker> const &, Frame const &) = 0;         // 1003
    virtual void operator()(Trace<deribit_multicast::Snapshot> const &, Frame const &) = 0;       // 1004
    virtual void operator()(Trace<deribit_multicast::SnapshotStart> const &, Frame const &) = 0;  // 1005
    virtual void operator()(Trace<deribit_multicast::SnapshotEnd> const &, Frame const &) = 0;    // 1006
    virtual void operator()(Trace<deribit_multicast::ComboLegs> const &, Frame const &) = 0;      // 1007
    virtual void operator()(Trace<deribit_multicast::PriceIndex> const &, Frame const &) = 0;     // 1008
    virtual void operator()(Trace<deribit_multicast::Rfq> const &, Frame const &) = 0;            // 1009
    virtual void operator()(Trace<deribit_multicast::InstrumentV2> const &, Frame const &) = 0;   // 1010
  };

  static bool dispatch(Handler &, std::span<std::byte const> const &buffer, TraceInfo const &);
};

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
