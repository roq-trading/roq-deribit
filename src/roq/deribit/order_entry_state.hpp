/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <cstdint>

namespace roq {
namespace deribit {

enum class OrderEntryState : uint8_t {
  UNDEFINED = 0,
  POSITIONS,
  ORDERS,
  DONE,
};

}  // namespace deribit
}  // namespace roq
