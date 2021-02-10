/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

namespace roq {
namespace deribit {
namespace fix {

enum class AdvOrderType : char {
  UNKNOWN = '\0',
  IMPLIED_VOLATILITY_ORDER = '0',
  USD_ORDER = '1',
};

extern std::string_view EnumNameAdvOrderType(const AdvOrderType &value);

extern AdvOrderType parse_adv_order_type(const std::string_view &value);

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::AdvOrderType> {
  template <typename C>
  constexpr auto parse(C &ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::AdvOrderType &value, C &ctx) {
    using namespace std::literals;  // NOLINT
    return format_to(ctx.out(), "{}"sv, roq::deribit::fix::EnumNameAdvOrderType(value));
  }
};
