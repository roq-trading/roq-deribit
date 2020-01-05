/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <gflags/gflags.h>

DECLARE_string(listen);
DECLARE_string(config_file);

DECLARE_string(ws_uri);
DECLARE_string(fix_uri);
DECLARE_uint64(ping_freq_secs);
DECLARE_string(exchange);
DECLARE_bool(cancel_on_disconnect);
DECLARE_bool(silence_empty_messages);
DECLARE_uint32(max_trades);
DECLARE_uint32(encode_buffer_size);
DECLARE_uint32(decode_buffer_size);
DECLARE_uint64(reconnect_secs);

// following options are work-arounds for weird behavior:
// - batch subscription doesn't seem to work (as of 2019-10-06)
DECLARE_bool(batch_subscribe);
DECLARE_uint32(max_batch_size);

DECLARE_bool(log_fix);

// external

DECLARE_string(name);
DECLARE_uint32(max_depth);
