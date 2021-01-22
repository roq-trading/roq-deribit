/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <absl/flags/declare.h>

#include <cstdint>
#include <string>

ABSL_DECLARE_FLAG(std::string, config_file);

ABSL_DECLARE_FLAG(std::string, exchange);

// ws
ABSL_DECLARE_FLAG(std::string, ws_uri);
ABSL_DECLARE_FLAG(uint32_t, ws_ping_freq_secs);
ABSL_DECLARE_FLAG(uint32_t, ws_request_timeout_secs);

// fix
ABSL_DECLARE_FLAG(std::string, fix_uri);
ABSL_DECLARE_FLAG(uint32_t, fix_ping_freq_secs);
ABSL_DECLARE_FLAG(uint32_t, fix_request_timeout_secs);
ABSL_DECLARE_FLAG(bool, fix_cancel_on_disconnect);
ABSL_DECLARE_FLAG(uint32_t, fix_market_data_request_max_size);
ABSL_DECLARE_FLAG(bool, fix_debug);

// XXX review
ABSL_DECLARE_FLAG(uint32_t, encode_buffer_size);
ABSL_DECLARE_FLAG(uint32_t, decode_buffer_size);

// external
ABSL_DECLARE_FLAG(std::string, name);
ABSL_DECLARE_FLAG(uint32_t, cache_mbp_max_depth);
ABSL_DECLARE_FLAG(uint32_t, cache_trades_max_depth);
ABSL_DECLARE_FLAG(uint32_t, cache_fills_max_depth);
