/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/flags.h"

#include <absl/flags/declare.h>
#include <absl/flags/flag.h>

#include <string>

ABSL_FLAG(std::string, config_file, "", "config file (path)");

ABSL_FLAG(std::string, exchange, "deribit", "exchange identifier (string)");

// ws

ABSL_FLAG(
    std::string,
    ws_uri,
    "wss://test.deribit.com/ws/api/v2",
    "WebSocket end-point (URI)");

ABSL_FLAG(uint32_t, ws_ping_freq_secs, 5, "ping frequency (seconds)");

ABSL_FLAG(uint32_t, ws_request_timeout_secs, 15, "request timeout (seconds)");

// fix

ABSL_FLAG(
    std::string, fix_uri, "tcp://test.deribit.com:9881", "FIX end-point (URI)");

ABSL_FLAG(uint32_t, fix_ping_freq_secs, 5, "ping frequency (seconds)");

ABSL_FLAG(uint32_t, fix_request_timeout_secs, 15, "request timeout (seconds)");

ABSL_FLAG(
    bool,
    fix_cancel_on_disconnect,
    true,
    "cancel orders on disconnect? (bool)");

ABSL_FLAG(
    uint32_t,
    fix_market_data_request_max_size,
    56,
    "max batch size (it appears there is a limit)");

ABSL_FLAG(bool, fix_debug, false, "log fix messages?");

// XXX review

ABSL_FLAG(uint32_t, encode_buffer_size, 1048576, "encode buffer size");

ABSL_FLAG(uint32_t, decode_buffer_size, 10485760, "decode buffer size");

// external

ABSL_DECLARE_FLAG(std::string, name);
ABSL_DECLARE_FLAG(uint32_t, cache_mbp_max_depth);
ABSL_DECLARE_FLAG(uint32_t, cache_trades_max_depth);
ABSL_DECLARE_FLAG(uint32_t, cache_fills_max_depth);

namespace roq {
namespace deribit {

std::string_view Flags::config_file() {
  static const std::string result = absl::GetFlag(FLAGS_config_file);
  return result;
}

std::string_view Flags::exchange() {
  static const std::string result = absl::GetFlag(FLAGS_exchange);
  return result;
}

std::string_view Flags::ws_uri() {
  static const std::string result = absl::GetFlag(FLAGS_ws_uri);
  return result;
}

uint32_t Flags::ws_ping_freq_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_ws_ping_freq_secs);
  return result;
}

uint32_t Flags::ws_request_timeout_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_ws_request_timeout_secs);
  return result;
}

std::string_view Flags::fix_uri() {
  static const std::string result = absl::GetFlag(FLAGS_fix_uri);
  return result;
}

uint32_t Flags::fix_ping_freq_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_fix_ping_freq_secs);
  return result;
}

uint32_t Flags::fix_request_timeout_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_fix_request_timeout_secs);
  return result;
}

bool Flags::fix_cancel_on_disconnect() {
  static const bool result = absl::GetFlag(FLAGS_fix_cancel_on_disconnect);
  return result;
}

uint32_t Flags::fix_market_data_request_max_size() {
  static const uint32_t result =
      absl::GetFlag(FLAGS_fix_market_data_request_max_size);
  return result;
}

bool Flags::fix_debug() {
  static const bool result = absl::GetFlag(FLAGS_fix_debug);
  return result;
}

uint32_t Flags::encode_buffer_size() {
  static const uint32_t result = absl::GetFlag(FLAGS_encode_buffer_size);
  return result;
}

uint32_t Flags::decode_buffer_size() {
  static const uint32_t result = absl::GetFlag(FLAGS_decode_buffer_size);
  return result;
}

std::string_view Flags::name() {
  static const std::string result = absl::GetFlag(FLAGS_name);
  return result;
}

uint32_t Flags::cache_mbp_max_depth() {
  static const uint32_t result = absl::GetFlag(FLAGS_cache_mbp_max_depth);
  return result;
}

uint32_t Flags::cache_trades_max_depth() {
  static const uint32_t result = absl::GetFlag(FLAGS_cache_trades_max_depth);
  return result;
}

uint32_t Flags::cache_fills_max_depth() {
  static const uint32_t result = absl::GetFlag(FLAGS_cache_fills_max_depth);
  return result;
}

}  // namespace deribit
}  // namespace roq
