/* Copyright (c) 2017-2026, Hans Erik Thrane */

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

extern std::string_view EnumNameAdvOrderType(AdvOrderType const &value);

extern AdvOrderType parse_adv_order_type(std::string_view const &value);

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::AdvOrderType> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::deribit::fix::AdvOrderType const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{}"sv, roq::deribit::fix::EnumNameAdvOrderType(value));
  }
};
