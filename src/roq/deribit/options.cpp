/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/options.h"

#include <absl/flags/flag.h>

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
