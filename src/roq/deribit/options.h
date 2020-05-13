/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <gflags/gflags.h>

DECLARE_string(config_file);

DECLARE_string(exchange);

DECLARE_uint32(download_timeout_secs);

DECLARE_string(ws_uri);
DECLARE_uint32(ws_ping_freq_secs);

DECLARE_string(fix_uri);
DECLARE_uint32(fix_ping_freq_secs);
DECLARE_bool(fix_cancel_on_disconnect);
DECLARE_bool(fix_debug);

// XXX review

DECLARE_uint32(encode_buffer_size);
DECLARE_uint32(decode_buffer_size);

// workarounds

DECLARE_uint32(max_batch_size);

// external

DECLARE_string(name);
DECLARE_uint32(cache_mbp_max_depth);
DECLARE_uint32(cache_trades_max_depth);
DECLARE_uint32(cache_fills_max_depth);
