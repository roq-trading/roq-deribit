/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/strategy/application.hpp"

#include "roq/logging.hpp"

#include "roq/deribit/gateway/config.hpp"

#include "roq/deribit/strategy/controller.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace strategy {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  gateway::Config config{settings};
  log::warn("config={}"sv, config);
  auto context = server::create_io_context(settings);
  Controller{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace strategy
}  // namespace deribit
}  // namespace roq
