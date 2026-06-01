/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/strategy/application.hpp"

#include <chrono>

#include "roq/logging.hpp"

#include "roq/io/sys/scheduler.hpp"

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
  controller.start();
  std::chrono::nanoseconds next_yield_ = {};
  auto ok = true;
  while (ok) {
    auto now = clock::get_system();
    controller.refresh(now);
    if (next_yield_ < now && YIELD_FREQUENCY.count() > 0) {
      next_yield_ = now + YIELD_FREQUENCY;
      io::sys::Scheduler::yield();
    }
    for (size_t i = 0; ok && i < DISPATCH_THIS_MANY_BEFORE_CHECKING_CLOCK; ++i) {
      ok = controller.dispatch();
    }
  }
  controller.stop();
  return EXIT_SUCCESS;
}

}  // namespace strategy
}  // namespace deribit
}  // namespace roq
