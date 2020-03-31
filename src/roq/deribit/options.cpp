/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/options.h"

DEFINE_string(listen,
    "",
    "bind address (path)");
// DEFINE_validator(listen, ...);

DEFINE_string(config_file,
    "",
    "config file (path)");

DEFINE_string(ws_uri,
    "wss://test.deribit.com/ws/api/v2",
    "WebSocket end-point (URI)");

DEFINE_string(fix_uri,
    "tcp://test.deribit.com:9881",
    "FIX end-point (URI)");

DEFINE_uint32(rate_limit_interval_secs,
    uint32_t{10},
    "rate limit: monitor interval (seconds)");

DEFINE_uint32(rate_limit_max_requests,
    uint32_t{20},
    "rate limit: max requests (per interval)");

DEFINE_uint32(ping_freq_secs,
    uint32_t{5},
    "ping frequency (seconds)");

DEFINE_uint32(download_timeout_secs,
    uint32_t{5},
    "download time-out (seconds)");

DEFINE_string(exchange,
    "deribit",
    "exchange identifier (string)");

DEFINE_bool(cancel_on_disconnect,
    true,
    "cancel orders on disconnect? (bool)");

DEFINE_uint32(max_trades,
    uint32_t{16384},
    "maximum trades for trade summary");

DEFINE_uint32(max_fills,
    uint32_t{256},
    "maximum fills for trade update");

DEFINE_uint32(encode_buffer_size,
    uint32_t{1048576},
    "encode buffer size");

DEFINE_uint32(decode_buffer_size,
    uint32_t{10485760},
    "decode buffer size");

DEFINE_uint32(reconnect_secs,
    {10},
    "time before reconnect (seconds)");

DEFINE_uint32(max_batch_size,
    56,
    "max batch size (it appears there is a limit)");

DEFINE_bool(log_fix,
    false,
    "log fix messages?");
