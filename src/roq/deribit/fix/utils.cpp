/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/fix/utils.h"

using namespace roq::literals;

namespace roq {
namespace deribit {
namespace fix {

SecurityType map_security_type(const std::string_view &value) {
  if (value.length() == 3) {
    switch (value.data()[0]) {
      case 'F':
        if (value.compare("FUT"_sv) == 0)
          return SecurityType::FUTURES;
        break;
      case 'O':
        if (value.compare("OPT"_sv) == 0)
          return SecurityType::OPTION;
        break;
    }
  }
  return SecurityType::UNDEFINED;
}

Error map_error(const std::string_view &value) {
  if (value.length() > 0) {
    switch (value.data()[0]) {
      case 'c':
        if (value.compare("canceled"_sv) == 0)
          return Error::UNDEFINED;
        break;
      case 's':
        if (value.compare("success"_sv) == 0)
          return Error::UNDEFINED;
        break;
    }
  }
  return Error::UNKNOWN;
}

std::string_view map(ExecutionInstruction execution_instruction) {
  switch (execution_instruction) {
    case ExecutionInstruction::UNDEFINED:
      return std::string_view();
    case ExecutionInstruction::PARTICIPATE_DO_NOT_INITIATE:
      return "6"_sv;
    case ExecutionInstruction::DO_NOT_INCREASE:
      return "E"_sv;
    default:
      throw RuntimeErrorException("Not a supported execution instruction"_sv);
  }
}

Error reject_to_error(const std::string_view &reason, const std::string_view &text) {
  if (reason.empty() && text.compare("rate_limit_exceeded"_sv) == 0)
    return Error::REQUEST_RATE_LIMIT_REACHED;
  return {};
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
