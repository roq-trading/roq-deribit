/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>

#include "roq/market/mbp/sequencer.hpp"

namespace roq {
namespace deribit {

struct Instrument final {
  Instrument(std::string_view const &symbol, double contract_size, double multiplier, bool discard);

  std::string symbol;
  double contract_size = NaN;
  double multiplier = NaN;
  bool discard = {};
  market::mbp::Sequencer mbp_sequencer;
};

}  // namespace deribit
}  // namespace roq
