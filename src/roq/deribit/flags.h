/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <cstdint>
#include <string_view>

namespace roq {
namespace deribit {

struct Flags final {
  static std::string_view config_file();
  static std::string_view exchange();
  static std::string_view ws_uri();
  static uint32_t ws_ping_freq_secs();
  static uint32_t ws_request_timeout_secs();
  static std::string_view fix_uri();
  static uint32_t fix_ping_freq_secs();
  static uint32_t fix_request_timeout_secs();
  static bool fix_cancel_on_disconnect();
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
