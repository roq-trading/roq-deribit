/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/options.h"

DEFINE_string(config_file, "", "config file (path)");

DEFINE_string(exchange, "deribit", "exchange identifier (string)");

// ws

DEFINE_string(
    ws_uri, "wss://test.deribit.com/ws/api/v2", "WebSocket end-point (URI)");

DEFINE_uint32(ws_ping_freq_secs, 5, "ping frequency (seconds)");

DEFINE_uint32(ws_request_timeout_secs, 15, "request timeout (seconds)");

// fix

DEFINE_string(fix_uri, "tcp://test.deribit.com:9881", "FIX end-point (URI)");

DEFINE_uint32(fix_ping_freq_secs, 5, "ping frequency (seconds)");

DEFINE_uint32(fix_request_timeout_secs, 15, "request timeout (seconds)");

DEFINE_bool(
    fix_cancel_on_disconnect, true, "cancel orders on disconnect? (bool)");

DEFINE_uint32(
    fix_market_data_request_max_size,
    56,
    "max batch size (it appears there is a limit)");

DEFINE_bool(fix_debug, false, "log fix messages?");

// XXX review

DEFINE_uint32(encode_buffer_size, 1048576, "encode buffer size");

DEFINE_uint32(decode_buffer_size, 10485760, "decode buffer size");
