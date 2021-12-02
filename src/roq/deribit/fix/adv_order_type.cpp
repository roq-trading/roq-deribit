/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/fix/adv_order_type.h"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace fix {

std::string_view EnumNameAdvOrderType(const AdvOrderType &value) {
  switch (value) {
    case AdvOrderType::IMPLIED_VOLATILITY_ORDER:
      return "IMPLIED_VOLATILITY_ORDER"sv;
    case AdvOrderType::USD_ORDER:
      return "USD_ORDER"sv;
    default:
      return "UNKNOWN"sv;
  }
}

AdvOrderType parse_adv_order_type(const std::string_view &value) {
  if (std::empty(value))
    return AdvOrderType::UNKNOWN;
  switch (std::data(value)[0]) {
    case '0':
      return AdvOrderType::IMPLIED_VOLATILITY_ORDER;
    case '1':
      return AdvOrderType::USD_ORDER;
    default:
      return AdvOrderType::UNKNOWN;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
