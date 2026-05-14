/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <cstdint>

namespace roq {
namespace deribit {

enum class DropCopyState : uint8_t {
  UNDEFINED = 0,
  SUBSCRIBE_USER_PORTFOLIO,
  SUBSCRIBE_USER_CHANGES,
  SUBSCRIBE_USER_ORDERS,
  SUBSCRIBE_USER_TRADES,
  GET_ACCOUNT_SUMMARY,
  GET_USER_TRADES_BY_CURRENCY,
  DONE,
};

}  // namespace deribit
}  // namespace roq
