/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/json/message.h"

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

constexpr Message::Field Message::parse_name(const std::string_view& name) {
  if (name.empty())
    return Field::UNKNOWN;
  switch (name.data()[0]) {
    case 'e':
      if (name.compare("error") == 0)
        return Field::ERROR;
      break;
    case 'i':
      if (name.compare("id") == 0)
        return Field::ID;
      break;
    case 'j':
      if (name.compare("jsonrpc") == 0)
        return Field::JSONRPC;
      break;
    case 'm':
      if (name.compare("method") == 0)
        return Field::METHOD;
      break;
    case 'p':
      if (name.compare("params") == 0)
        return Field::PARAMS;
      break;
    case 'r':
      if (name.compare("result") == 0)
        return Field::RESULT;
      break;
    case 't':
      if (name.compare("testnet") == 0)
        return Field::TESTNET;
      break;
    case 'u':
      if (name.length() >= 3) {
        switch (name.data()[2]) {
          case 'D':
            if (name.compare("usDiff") == 0)
              return Field::US_DIFF;
            break;
          case 'I':
            if (name.compare("usIn") == 0)
              return Field::US_IN;
            break;
          case 'O':
            if (name.compare("usOut") == 0)
              return Field::US_OUT;
            break;
        }
      }
      break;
  }
  return Field::UNKNOWN;
}

static_assert(Message::parse_name("error") == Message::Field::ERROR);
static_assert(Message::parse_name("id") == Message::Field::ID);
static_assert(Message::parse_name("jsonrpc") == Message::Field::JSONRPC);
static_assert(Message::parse_name("method") == Message::Field::METHOD);
static_assert(Message::parse_name("params") == Message::Field::PARAMS);
static_assert(Message::parse_name("result") == Message::Field::RESULT);
static_assert(Message::parse_name("testnet") == Message::Field::TESTNET);
static_assert(Message::parse_name("usDiff") == Message::Field::US_DIFF);
static_assert(Message::parse_name("usIn") == Message::Field::US_IN);
static_assert(Message::parse_name("usOut") == Message::Field::US_OUT);

}  // namespace json
}  // namespace deribit
}  // namespace roq
