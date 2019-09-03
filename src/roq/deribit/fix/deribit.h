/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <cstdint>
#include <string_view>

namespace roq {
namespace deribit {
namespace fix {

enum class Deribit : uint32_t {
  INSTRUMENT_PRICE_PRECISION = 2576,
  CONDITION_TRIGGER_METHOD = 5127,
  CANCEL_ON_DISCONNECT = 9001,
  USE_WORDSAFE_TAGS = 9002,
  TRADE_AMOUNT = 100007,
  SINCE_TIMESTAMP = 100008,
  TRADE_ID = 100009,
  LABEL = 100010,
  ADV_ORDER_TYPE = 100012,
  TRADE_VOLUME_24H = 100087,
  MARK_PRICE = 100090,
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
        ctx.begin(),
        "{}",
        roq::deribit::fix::EnumNameAdvOrderType(value));
  }
};
