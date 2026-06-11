/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/fix_bridge/application.hpp"

#include "roq/logging.hpp"

#include "roq/deribit/gateway/config.hpp"

#include "roq/deribit/fix_bridge/controller.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace fix_bridge {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  gateway::Config config{settings};
  log::warn("config={}"sv, config);
  auto context = server::create_io_context(settings);
  Controller{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace fix_bridge
}  // namespace deribit
}  // namespace roq
