/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/api.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

inline void update(
    AdvOrderType& result,
    const std::string_view& value) {
  result = parse_adv_order_type(value);
}

SecurityType map_security_type(const std::string_view& value);

Error map_error(const std::string_view& value);

}  // namespace fix
}  // namespace deribit
}  // namespace roq
