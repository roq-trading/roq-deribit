/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

#include "roq/core/uri.h"

namespace roq {
namespace deribit {

struct Flags final {
  static std::string_view config_file();
  static std::string_view exchange();
  static const core::URI &ws_uri();
  static std::chrono::seconds ws_ping_freq();
  static std::chrono::seconds ws_request_timeout();
  static const core::URI &fix_uri();
  static std::chrono::seconds fix_ping_freq();
  static std::chrono::seconds fix_request_timeout();
  static bool fix_cancel_on_disconnect();
  static uint32_t fix_market_data_max_subscriptions_per_stream();
  static uint32_t fix_market_data_request_max_size();
  static bool fix_debug();
  static uint32_t encode_buffer_size();
  static uint32_t decode_buffer_size();
  // external
  static std::string_view name();
  static uint32_t cache_mbp_max_depth();
  static uint32_t cache_trades_max_depth();
  static uint32_t cache_fills_max_depth();
};

}  // namespace deribit
}  // namespace roq
