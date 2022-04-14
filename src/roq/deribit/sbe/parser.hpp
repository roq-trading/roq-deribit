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

namespace roq {
namespace deribit {
namespace sbe {

struct Parser final {
  struct Handler {
    // events
    virtual void operator()(
        uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Instrument &) = 0;
    virtual void operator()(
        uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Book &) = 0;
    virtual void operator()(
        uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Quote &) = 0;
    virtual void operator()(
        uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Trades &) = 0;
    // snapshot
    virtual void operator()(
        uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Snapshot &) = 0;
  };

  static bool dispatch(Handler &, const std::span<std::byte const> &buffer);
};

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
