/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/filter/application.hpp"

#include "roq/logging.hpp"

#include "roq/deribit/filter/controller.hpp"
#include "roq/deribit/filter/settings.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace filter {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  Controller{settings}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace filter
}  // namespace deribit
}  // namespace roq
