/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <vector>

#include "roq/mbp_update.hpp"

#include "roq/deribit/sbe/frame.hpp"

namespace roq {
namespace deribit {

struct Channel final {
  Channel() = default;

  Channel(Channel &&) = default;
  Channel(Channel const &) = delete;

  uint32_t previous_sequence_number() const { return previous_sequence_number_; }

  bool operator()(sbe::Frame const &);

  void reset(sbe::Frame const &);

  // snapshot

  void snapshot_start(sbe::Frame const &);
  void snapshot_end(sbe::Frame const &);

  bool ready() const { return ready_; }

 public:
  uint32_t instrument_id = {};
  std::vector<MBPUpdate> bids, asks;  // note! book/snapshot update may span multiple datagrams

 private:
  uint32_t previous_sequence_number_ = {};
  bool initialized_ = {};
  bool ready_ = {};
};

}  // namespace deribit
}  // namespace roq
