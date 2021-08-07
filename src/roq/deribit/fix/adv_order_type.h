/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/literals.h"

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
  template <typename Context>
  constexpr auto parse(Context &context) {
    return context.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::AdvOrderType &value, C &ctx) {
    using namespace roq::literals;
    return fmt::format_to(ctx.out(), "{}"_sv, roq::deribit::fix::EnumNameAdvOrderType(value));
  }
};
