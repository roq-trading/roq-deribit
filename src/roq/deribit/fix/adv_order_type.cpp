/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/adv_order_type.h"

namespace roq {
namespace deribit {
namespace fix {

const char *EnumNameAdvOrderType(const AdvOrderType &value) {
  switch (value) {
    case AdvOrderType::IMPLIED_VOLATILITY_ORDER:
      return "IMPLIED_VOLATILITY_ORDER";
    case AdvOrderType::USD_ORDER: return "USD_ORDER";
    default: return "UNKNOWN";
  }
}

AdvOrderType parse_adv_order_type(const std::string_view &value) {
  if (value.empty())
    return AdvOrderType::UNKNOWN;
  switch (value.data()[0]) {
    case '0': return AdvOrderType::IMPLIED_VOLATILITY_ORDER;
    case '1': return AdvOrderType::USD_ORDER;
    default: return AdvOrderType::UNKNOWN;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
