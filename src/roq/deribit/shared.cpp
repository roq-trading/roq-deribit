/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/shared.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const BUFFER_SIZE = 4096uz;
}

// === HELPERS ===

namespace {
auto get_multicast(auto &settings) {
  // XXX maybe check more flags?
  if (std::empty(settings.multicast.local_interface)) {
    return false;
  }
  if (std::empty(settings.multicast.channel_ids)) {
    log::fatal("Unexpected: --channel_ids is empty"sv);
  }
  if (std::empty(settings.multicast.config_file)) {
    log::fatal("Unexpected: --config_file is empty"sv);
  }
  log::info("Using multicast"sv);
  return true;
}

template <typename R>
auto create_sbe_config(auto &settings) {
  using result_type = std::remove_cvref_t<R>;
  return result_type{settings.multicast.config_file, settings.multicast.channel_ids};
}
}  // namespace

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : api{API::create(settings)}, dispatcher{dispatcher}, settings{settings}, multicast_{get_multicast(settings)},
      rate_limiter{settings.request.limit, settings.request.limit_interval}, symbols{settings.fix.market_data_max_subscriptions_per_stream},
      buffer(BUFFER_SIZE), sbe_config{create_sbe_config<decltype(sbe_config)>(settings)} {
}

std::string_view Shared::next_request_id() {
  auto request_id = ++request_id_;
  request_id_encode_buffer_.clear();
  fmt::format_to(std::back_inserter(request_id_encode_buffer_), "roq-{}"sv, request_id);
  return request_id_encode_buffer_;
}

}  // namespace deribit
}  // namespace roq
