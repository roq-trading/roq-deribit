/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/application.hpp"

#include "roq/deribit/flags/settings.hpp"

#include "roq/deribit/gateway/config.hpp"
#include "roq/deribit/gateway/controller.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  flags::Settings settings{args};
  gateway::Config config{settings};
  auto context = server::create_io_context(settings);
  server::Trading<gateway::Controller>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace deribit
}  // namespace roq
