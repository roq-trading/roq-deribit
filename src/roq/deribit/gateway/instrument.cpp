/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/gateway/instrument.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace gateway {

// === IMPLEMENTATION ===

Instrument::Instrument(std::string_view const &symbol, double contract_size, double multiplier, bool discard)
    : symbol{symbol}, contract_size{contract_size}, multiplier{multiplier}, discard{discard} {
}

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
