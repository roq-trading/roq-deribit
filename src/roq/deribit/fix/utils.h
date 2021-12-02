/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/api.h"

#include "roq/deribit/fix/adv_order_type.h"

namespace roq {
namespace deribit {
namespace fix {

inline void update(AdvOrderType &result, const std::string_view &value) {
  result = parse_adv_order_type(value);
}

SecurityType map_security_type(const std::string_view &value);

Error map_error(const std::string_view &value);

std::string_view map(ExecutionInstruction execution_instruction);

extern Error reject_to_error(const std::string_view &reason, const std::string_view &text);

}  // namespace fix
}  // namespace deribit
}  // namespace roq
