/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/application.hpp"

#include "roq/deribit/config.hpp"
#include "roq/deribit/gateway.hpp"
#include "roq/deribit/settings.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const TYPE = server::Type::ORDER_MANAGEMENT;
}

// === IMPLEMENTATION ===

int Application::main(int, char **) {
  Settings settings{TYPE};
  Config config{settings};
  auto context = server::create_io_context();
  server::Trading<Gateway>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace deribit
}  // namespace roq
