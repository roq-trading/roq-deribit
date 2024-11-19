/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/deribit/dump/settings.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace dump {

// === IMPLEMENTATION ===

Settings::Settings(args::Parser const &) : flags::Flags{flags::Flags::create()} {
}

}  // namespace dump
}  // namespace deribit
}  // namespace roq
