/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/filter/settings.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace filter {

// === IMPLEMENTATION ===

Settings::Settings(args::Parser const &) : flags::Flags{flags::Flags::create()}, multicast{flags::multicast::create()} {
}

}  // namespace filter
}  // namespace deribit
}  // namespace roq
