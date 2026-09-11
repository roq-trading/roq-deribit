/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/protocol/fix/utils.hpp"

#include "roq/utils/hash/fnv.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace protocol {
namespace fix {

// === CONSTANTS ===

namespace {}

// === IMPLEMENTATION ===

SecurityType map_security_type(std::string_view const &value) {
  if (!std::empty(value)) {
    auto key = utils::hash::FNV::compute(value);
    switch (key) {
      case utils::hash::FNV::compute("FUT"sv):
        return SecurityType::FUTURES;
      case utils::hash::FNV::compute("OPT"sv):
        return SecurityType::OPTION;
      case utils::hash::FNV::compute("FXSPOT"sv):
        return SecurityType::SPOT;
    }
  }
  return SecurityType::UNDEFINED;
}

Error map_error(std::string_view const &value) {
  if (!std::empty(value)) {
    auto key = utils::hash::FNV::compute(value);
    switch (key) {
      case utils::hash::FNV::compute("already_cancelled"sv):
        return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
      case utils::hash::FNV::compute("canceled"sv):
        return Error::UNDEFINED;
      case utils::hash::FNV::compute("not_found"sv):
        return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
      case utils::hash::FNV::compute("rejected: order is closed"sv):
        return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
      case utils::hash::FNV::compute("rejected: settlement_in_progress"sv):
        return Error::SETTLEMENT_IN_PROGRESS;
      case utils::hash::FNV::compute("success"sv):
        return Error::UNDEFINED;
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
}  // namespace protocol
}  // namespace deribit
}  // namespace roq
