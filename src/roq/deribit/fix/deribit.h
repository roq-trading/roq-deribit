/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <cstdint>
#include <string_view>

#include "roq/core/fix/common.h"

namespace roq {
namespace deribit {
namespace fix {

constexpr auto FIX_VERSION = core::fix::Version::FIX_44;
constexpr auto SENDER_COMP_ID = "ROQ_TRADING";
constexpr auto TARGET_COMP_ID = "DERIBITSERVER";

enum class Deribit : uint32_t {
  INSTRUMENT_PRICE_PRECISION = 2576,
  CONDITION_TRIGGER_METHOD = 5127,
  CANCEL_ON_DISCONNECT = 9001,
  USE_WORDSAFE_TAGS = 9002,
  USER_EQUITY = 100001,
  USER_BALANCE = 100002,
  USER_INITIAL_MARGIN = 100003,
  USER_MAINTENANCE_MARGIN = 100004,
  UNREALIZED_PL = 100005,
  REALIZED_PL = 100006,
  TRADE_AMOUNT = 100007,
  SINCE_TIMESTAMP = 100008,
  TRADE_ID = 100009,
  LABEL = 100010,
  TOTAL_PL = 100011,
  ADV_ORDER_TYPE = 100012,
  MARGIN_BALANCE = 100013,
  TRADE_VOLUME_24H = 100087,
  LIQUIDATION_PRICE = 100088,
  SIZE_IN_CURRENCY = 100089,
  MARK_PRICE = 100090,
  LIQUIDATION = 100091,
  TODO_1 = 100092,
  TODO_2 = 100093,
};

enum class AdvOrderType : char {
  UNKNOWN = '\0',
  IMPLIED_VOLATILITY_ORDER = '0',
  USD_ORDER = '1',
};

extern const char *EnumNameAdvOrderType(const AdvOrderType& value);

extern AdvOrderType parse_adv_order_type(const std::string_view& value);

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::AdvOrderType> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::AdvOrderType& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{}",
        roq::deribit::fix::EnumNameAdvOrderType(value));
  }
};
