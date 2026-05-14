/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <cstdint>

namespace roq {
namespace deribit {

enum class WebSocketState : uint8_t {
  UNDEFINED = 0,
  CURRENCIES,
  INSTRUMENTS,
  SUBSCRIBE,
  DONE,
};

}  // namespace deribit
}  // namespace roq
