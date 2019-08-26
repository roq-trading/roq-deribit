/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <limits>
#include <string_view>

#include "roq/core/json/parser.h"

namespace roq {
namespace deribit {
namespace json {

struct Currency final {
  std::string_view coin_type;
  std::string_view currency;
  std::string_view currency_long;
  bool disabled_deposit_address_creation = false;
  uint32_t fee_precision = 0;
  uint32_t min_confirmations = 0;
  double min_withdrawal_fee = std::numeric_limits<double>::quiet_NaN();
  double withdrawal_fee = std::numeric_limits<double>::quiet_NaN();
  // TODO(thraneh): withdrawal_priorities

  static void parse(Currency&, core::json::object_t&&);
};

}  // namespace json
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::json::Currency> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::json::Currency& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "coin_type=\"{}\", "
        "currency=\"{}\", "
        "currency_long={}, "
        "disabled_deposit_address_creation={}, "
        "fee_precision={}, "
        "min_confirmations={}, "
        "min_withdrawal_fee={}, "
        "withdrawal_fee={}"
        "}}",
        value.coin_type,
        value.currency,
        value.currency_long,
        value.disabled_deposit_address_creation,
        value.fee_precision,
        value.min_confirmations,
        value.min_withdrawal_fee,
        value.withdrawal_fee);
  }
};
