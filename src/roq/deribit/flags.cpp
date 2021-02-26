/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/flags.h"

#include <absl/flags/declare.h>
#include <absl/flags/flag.h>

#include <string>

#include "roq/core/flags/non_empty.h"
#include "roq/core/flags/non_zero.h"
#include "roq/core/flags/path.h"
#include "roq/core/flags/uri.h"

using namespace roq::literals;

ABSL_FLAG(  //
    roq::core::flags::Path<std::string>,
    config_file,
    {},
    "config file (path)"_sv);

ABSL_FLAG(  //
    roq::core::flags::NonEmpty<std::string>,
    exchange,
    "deribit"_s,
    "exchange identifier (string)"_sv);

// ws

ABSL_FLAG(  //
    roq::core::flags::URI<std::string>,
    ws_uri,
    "wss://test.deribit.com/ws/api/v2"_s,
    "WebSocket end-point (URI)"_sv);

ABSL_FLAG(  //
    roq::core::flags::NonZero<uint32_t>,
    ws_ping_freq_secs,
    uint32_t{5},
    "ping frequency (seconds)"_sv);

ABSL_FLAG(  //
    uint32_t,
    ws_request_timeout_secs,
    uint32_t{15},
    "request timeout (seconds)"_sv);

// fix

ABSL_FLAG(  //
    roq::core::flags::URI<std::string>,
    fix_uri,
    "tcp://test.deribit.com:9881"_s,
    "FIX end-point (URI)"_sv);

ABSL_FLAG(  //
    roq::core::flags::NonZero<uint32_t>,
    fix_ping_freq_secs,
    uint32_t{5},
    "ping frequency (seconds)"_sv);

ABSL_FLAG(  //
    uint32_t,
    fix_request_timeout_secs,
    uint32_t{15},
    "request timeout (seconds)"_sv);

ABSL_FLAG(  //
    bool,
    fix_cancel_on_disconnect,
    true,
    "cancel orders on disconnect? (bool)"_sv);

ABSL_FLAG(  //
    uint32_t,
    fix_market_data_max_subscriptions_per_stream,
    uint32_t{512},
    "max subscriptions per connection (count)"_sv);

ABSL_FLAG(  //
    uint32_t,
    fix_market_data_request_max_size,
    uint32_t{56},
    "max batch size (it appears there is a limit)"_sv);

ABSL_FLAG(  //
    bool,
    fix_debug,
    false,
    "log fix messages?"_sv);

// XXX review

ABSL_FLAG(  //
    roq::core::flags::NonZero<uint32_t>,
    encode_buffer_size,
    uint32_t{1048576},
    "encode buffer size"_sv);

ABSL_FLAG(  //
    roq::core::flags::NonZero<uint32_t>,
    decode_buffer_size,
    uint32_t{10485760},
    "decode buffer size"_sv);

// external

ABSL_DECLARE_FLAG(roq::core::flags::NonEmpty<std::string>, name);
ABSL_DECLARE_FLAG(roq::core::flags::NonZero<uint32_t>, cache_mbp_max_depth);
ABSL_DECLARE_FLAG(roq::core::flags::NonZero<uint32_t>, cache_trades_max_depth);
ABSL_DECLARE_FLAG(roq::core::flags::NonZero<uint32_t>, cache_fills_max_depth);

namespace roq {
namespace deribit {

std::string_view Flags::config_file() {
  static const std::string result{absl::GetFlag(FLAGS_config_file)};
  return result;
}

std::string_view Flags::exchange() {
  static const std::string result{absl::GetFlag(FLAGS_exchange)};
  return result;
}

const core::URI &Flags::ws_uri() {
  static const core::URI result{absl::GetFlag(FLAGS_ws_uri)};
  return result;
}

std::chrono::seconds Flags::ws_ping_freq() {
  static const std::chrono::seconds result{absl::GetFlag(FLAGS_ws_ping_freq_secs)};
  return result;
}

std::chrono::seconds Flags::ws_request_timeout() {
  static const std::chrono::seconds result{absl::GetFlag(FLAGS_ws_request_timeout_secs)};
  return result;
}

const core::URI &Flags::fix_uri() {
  static const core::URI result{absl::GetFlag(FLAGS_fix_uri)};
  return result;
}

std::chrono::seconds Flags::fix_ping_freq() {
  static const std::chrono::seconds result{absl::GetFlag(FLAGS_fix_ping_freq_secs)};
  return result;
}

std::chrono::seconds Flags::fix_request_timeout() {
  static const std::chrono::seconds result{absl::GetFlag(FLAGS_fix_request_timeout_secs)};
  return result;
}

bool Flags::fix_cancel_on_disconnect() {
  static const bool result{absl::GetFlag(FLAGS_fix_cancel_on_disconnect)};
  return result;
}

uint32_t Flags::fix_market_data_max_subscriptions_per_stream() {
  static const uint32_t result{absl::GetFlag(FLAGS_fix_market_data_max_subscriptions_per_stream)};
  return result;
}

uint32_t Flags::fix_market_data_request_max_size() {
  static const uint32_t result{absl::GetFlag(FLAGS_fix_market_data_request_max_size)};
  return result;
}

bool Flags::fix_debug() {
  static const bool result{absl::GetFlag(FLAGS_fix_debug)};
  return result;
}

uint32_t Flags::encode_buffer_size() {
  static const uint32_t result{absl::GetFlag(FLAGS_encode_buffer_size)};
  return result;
}

uint32_t Flags::decode_buffer_size() {
  static const uint32_t result{absl::GetFlag(FLAGS_decode_buffer_size)};
  return result;
}

std::string_view Flags::name() {
  static const std::string result{absl::GetFlag(FLAGS_name)};
  return result;
}

uint32_t Flags::cache_mbp_max_depth() {
  static const uint32_t result{absl::GetFlag(FLAGS_cache_mbp_max_depth)};
  return result;
}

uint32_t Flags::cache_trades_max_depth() {
  static const uint32_t result{absl::GetFlag(FLAGS_cache_trades_max_depth)};
  return result;
}

uint32_t Flags::cache_fills_max_depth() {
  static const uint32_t result{absl::GetFlag(FLAGS_cache_fills_max_depth)};
  return result;
}

}  // namespace deribit
}  // namespace roq
