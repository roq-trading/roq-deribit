/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/strategy/application.hpp"

#include "roq/logging.hpp"

#include "roq/io/engine/context_factory.hpp"

#include "roq/deribit/gateway/config.hpp"

#include "roq/deribit/strategy/controller.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace strategy {

// === CONSTANTS ===

namespace {
auto const DISPATCH_THIS_MANY_BEFORE_CHECKING_CLOCK = 1000uz;
auto const YIELD_FREQUENCY = 1000ms;
}  // namespace

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  gateway::Config config{settings};
  auto context = server::create_io_context(settings);
  Controller controller{settings, config, *context};
  while (controller.dispatch()) {
  }
  /*
  log::info("Starting the dispatch loop..."sv);
  (*dispatcher_).start();
  std::chrono::nanoseconds next_yield_ = {};
  auto ok = true;
  while (ok) {
    auto now = clock::get_system();
    refresh(now);
    if (next_yield_ < now && YIELD_FREQUENCY.count() > 0) {
      next_yield_ = now + YIELD_FREQUENCY;
      io::sys::Scheduler::yield();
    }
    for (size_t i = 0; ok && i < DISPATCH_THIS_MANY_BEFORE_CHECKING_CLOCK; ++i) {
      ok = (*dispatcher_).dispatch(*this);
    }
  }
  log::info("The dispatch loop has stopped!"sv);
  */
  return EXIT_SUCCESS;
}

}  // namespace strategy
}  // namespace deribit
}  // namespace roq
