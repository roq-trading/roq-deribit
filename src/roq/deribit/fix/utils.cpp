/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/fix/utils.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace fix {

SecurityType map_security_type(std::string_view const &value) {
  if (std::size(value) == 3) {
    switch (std::data(value)[0]) {
      case 'F':
        if (value == "FUT"sv) {
          return SecurityType::FUTURES;
        }
        break;
      case 'O':
        if (value == "OPT"sv) {
          return SecurityType::OPTION;
        }
        break;
    }
  }
  if (value == "FXSPOT"sv) {
    return SecurityType::SPOT;
  }
  return SecurityType::UNDEFINED;
}

Error map_error(std::string_view const &value) {
  if (std::size(value) > 0) {
    switch (std::data(value)[0]) {
      case 'a':
        if (value == "already_cancelled"sv) {
          return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
        }
        break;
      case 'c':
        if (value == "canceled"sv) {
          return Error::UNDEFINED;
        }
        break;
      case 's':
        if (value == "success"sv) {
          return Error::UNDEFINED;
        }
        break;
      case 'r':
        if (value == "rejected: order is closed"sv) {
          return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
        }
        break;
    }
  }
  return Error::UNKNOWN;
}

std::string_view map(Mask<ExecutionInstruction> execution_instructions) {
  if (std::empty(execution_instructions)) {
    return {};
  }
  if (execution_instructions.has(ExecutionInstruction::PARTICIPATE_DO_NOT_INITIATE)) {
    return "6"sv;
  }
  if (execution_instructions.has(ExecutionInstruction::DO_NOT_INCREASE)) {
    return "E"sv;
  }
  throw RuntimeError{"Not a supported execution instruction"sv};
}

Error reject_to_error(std::string_view const &reason, std::string_view const &text) {
  if (std::empty(reason) && text == "rate_limit_exceeded"sv) {
    return Error::REQUEST_RATE_LIMIT_REACHED;
  }
  return {};
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
