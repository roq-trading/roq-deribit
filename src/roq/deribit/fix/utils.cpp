/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

SecurityType map_security_type(const std::string_view &value) {
  if (value.length() == 3) {
    switch (value.data()[0]) {
      case 'F':
        if (value.compare("FUT") == 0)
          return SecurityType::FUTURES;
        break;
      case 'O':
        if (value.compare("OPT") == 0)
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
        if (value.compare("canceled") == 0)
          return Error::UNDEFINED;
        break;
      case 's':
        if (value.compare("success") == 0)
          return Error::UNDEFINED;
        break;
    }
  }
  return Error::UNKNOWN;
}

std::string_view map(ExecutionInstruction execution_instruction) {
  switch (execution_instruction) {
    case ExecutionInstruction::UNDEFINED: return std::string_view();
    case ExecutionInstruction::PARTICIPATE_DO_NOT_INITIATE: return "6";
    case ExecutionInstruction::DO_NOT_INCREASE: return "E";
    default: throw std::runtime_error("Not a supported execution instruction");
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
