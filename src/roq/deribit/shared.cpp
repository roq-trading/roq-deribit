/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/deribit/shared.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

using namespace fmt::literals;

namespace roq {
namespace deribit {

// === HELPERS ===

namespace {
auto get_multicast(auto &settings) {
  // XXX maybe check more flags?
  if (std::empty(settings.common.local_interface))
    return false;
  log::info("Using multicast"sv);
  return true;
}
}  // namespace

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : dispatcher{dispatcher}, settings{settings}, multicast_{get_multicast(settings)},
      rate_limiter{settings.common.request_limit, settings.common.request_limit_interval},
      symbols{settings.fix.market_data_max_subscriptions_per_stream} {
}

std::string_view Shared::next_request_id() {
  auto request_id = ++request_id_;
  request_id_encode_buffer_.clear();
  fmt::format_to(std::back_inserter(request_id_encode_buffer_), "roq-{}"_cf, request_id);
  return request_id_encode_buffer_;
}

}  // namespace deribit
}  // namespace roq
