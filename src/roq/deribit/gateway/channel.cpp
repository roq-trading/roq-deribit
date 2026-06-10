/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/gateway/channel.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace gateway {

// === IMPLEMENTATION ===

bool Channel::operator()(protocol::sbe::Frame const &frame) {
  auto result = true;
  if (frame.sequence_number == (previous_sequence_number_ + 1)) [[likely]] {
    previous_sequence_number_ = frame.sequence_number;
  } else {
    if (frame.sequence_number == previous_sequence_number_) {
      log::debug("*** REPEAT ***"sv);
      result = false;
    } else {
      if (initialized_) {
        log::warn(
            "*** DETECTED PACKET DROP *** (channel_id={}, sequence_number={}, previous_sequence_number={})"sv,
            frame.channel_id,
            frame.sequence_number,
            previous_sequence_number_);
        result = false;
        ready_ = false;  // note!
      } else {
        assert(previous_sequence_number_ == 0);
        initialized_ = true;
      }
      previous_sequence_number_ = frame.sequence_number;
    }
  }
  return result;
}

void Channel::reset(protocol::sbe::Frame const &) {
  bids.clear();
  asks.clear();
  instrument_id = {};
  ready_ = {};
}

void Channel::snapshot_start(protocol::sbe::Frame const &) {
  ready_ = true;
}

void Channel::snapshot_end(protocol::sbe::Frame const &) {
  ready_ = false;
}

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
