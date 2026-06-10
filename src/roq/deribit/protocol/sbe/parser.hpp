/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <span>

#include <deribit/sbe/multicast/Book.h>           // 1001
#include <deribit/sbe/multicast/ComboLegs.h>      // 1007
#include <deribit/sbe/multicast/Instrument.h>     // 1000
#include <deribit/sbe/multicast/InstrumentV2.h>   // 1010
#include <deribit/sbe/multicast/PriceIndex.h>     // 1008
#include <deribit/sbe/multicast/Rfq.h>            // 1009
#include <deribit/sbe/multicast/Snapshot.h>       // 1004
#include <deribit/sbe/multicast/SnapshotEnd.h>    // 1006
#include <deribit/sbe/multicast/SnapshotStart.h>  // 1005
#include <deribit/sbe/multicast/Ticker.h>         // 1003
#include <deribit/sbe/multicast/Trades.h>         // 1002

#include "roq/trace.hpp"

#include "roq/deribit/protocol/sbe/frame.hpp"

namespace roq {
namespace deribit {
namespace protocol {
namespace sbe {

struct Parser final {
  struct Handler {
    virtual bool operator()(Frame const &) = 0;

    virtual void operator()(Trace<::deribit::sbe::multicast::Instrument> const &, Frame const &) = 0;     // 1000
    virtual void operator()(Trace<::deribit::sbe::multicast::Book> const &, Frame const &) = 0;           // 1001
    virtual void operator()(Trace<::deribit::sbe::multicast::Trades> const &, Frame const &) = 0;         // 1002
    virtual void operator()(Trace<::deribit::sbe::multicast::Ticker> const &, Frame const &) = 0;         // 1003
    virtual void operator()(Trace<::deribit::sbe::multicast::Snapshot> const &, Frame const &) = 0;       // 1004
    virtual void operator()(Trace<::deribit::sbe::multicast::SnapshotStart> const &, Frame const &) = 0;  // 1005
    virtual void operator()(Trace<::deribit::sbe::multicast::SnapshotEnd> const &, Frame const &) = 0;    // 1006
    virtual void operator()(Trace<::deribit::sbe::multicast::ComboLegs> const &, Frame const &) = 0;      // 1007
    virtual void operator()(Trace<::deribit::sbe::multicast::PriceIndex> const &, Frame const &) = 0;     // 1008
    virtual void operator()(Trace<::deribit::sbe::multicast::Rfq> const &, Frame const &) = 0;            // 1009
    virtual void operator()(Trace<::deribit::sbe::multicast::InstrumentV2> const &, Frame const &) = 0;   // 1010
  };

  static bool dispatch(Handler &, std::span<std::byte const> const &buffer, TraceInfo const &);
};

}  // namespace sbe
}  // namespace protocol
}  // namespace deribit
}  // namespace roq
