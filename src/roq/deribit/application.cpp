/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/deribit/application.hpp"

#include "roq/deribit/config.hpp"
#include "roq/deribit/gateway.hpp"
#include "roq/deribit/settings.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
uint8_t const API = {};
}

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  Config config{settings};
  auto context = server::create_io_context(settings);
  server::Trading<Gateway>{settings, config, *context, API}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace deribit
}  // namespace roq
