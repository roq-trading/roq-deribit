/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/shared.hpp"

#include "roq/logging.hpp"

#include "roq/deribit/flags/common.hpp"
#include "roq/deribit/flags/fix.hpp"
#include "roq/deribit/flags/multicast.hpp"

using namespace std::literals;

using namespace fmt::literals;

namespace roq {
namespace deribit {

// === HELPERS ===

namespace {
auto get_multicast() {
  // XXX maybe check more flags?
  if (std::empty(flags::Multicast::local_interface()))
    return false;
  log::info("Using multicast"sv);
  return true;
}
}  // namespace

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : dispatcher{dispatcher}, settings{settings}, multicast_{get_multicast()},
      rate_limiter{flags::Common::request_limit(), flags::Common::request_limit_interval()},
      symbols{flags::FIX::fix_market_data_max_subscriptions_per_stream()} {
}

std::string_view Shared::next_request_id() {
  auto request_id = ++request_id_;
  stack_buffer_.clear();
  fmt::format_to(std::back_inserter(stack_buffer_), "roq-{}"_cf, request_id);
  return {std::data(stack_buffer_), std::size(stack_buffer_)};
}

}  // namespace deribit
}  // namespace roq
