/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/json/message.h"

#include "roq/deribit/json/field.h"

namespace roq {
namespace deribit {
namespace json {

Message::Type Message::find_type(const std::string_view& message) {
  // FIXME(thraneh): *must* ensure we only match top-level
  auto has_id = message.find("\"id\":");
  if (has_id == message.npos)
    return Type::NOTIFICATION;
  auto has_result = message.find("\"result\":");
  if (has_result) {
    // strictly: valid only if error does *not* exist
    return Type::RESPONSE;
  }
  auto has_error = message.find("\"error\":");
  if (has_error) {
    // strictly: valid only if result does *not* exist
    return Type::ERROR;
  }
  return Type::UNKNOWN;
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
