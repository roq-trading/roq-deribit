/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/bridge/settings.hpp"

#include "roq/server/bridge/flags/settings.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace bridge {

// === HELPERS ===

namespace {
auto create_bridge(auto &parser) {
  return server::bridge::flags::Settings{parser};
}
}  // namespace

// === IMPLEMENTATION ===

Settings::Settings(args::Parser const &parser) : flags::Settings{parser}, bridge{create_bridge(parser)} {
}

}  // namespace bridge
}  // namespace deribit
}  // namespace roq
