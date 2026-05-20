/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/application.hpp"

#include "roq/deribit/flags/settings.hpp"

#include "roq/deribit/gateway/config.hpp"
#include "roq/deribit/gateway/controller.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
uint8_t const API_2 = {};
}

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  flags::Settings settings{args};
  gateway::Config config{settings};
  auto context = server::create_io_context(settings);
  server::Trading2<gateway::Controller>{settings, config, *context, API_2}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace deribit
}  // namespace roq
