/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/json/parser.h"

namespace roq {
namespace deribit {
namespace json {

struct response_t final {
  core::json::value_t result;
};

struct error_t final {
  uint32_t code;
  std::string_view message;
};

struct notification_t final {
  std::string_view method;
  std::string_view channel;
  core::json::value_t data;
};

class Message final {
 public:
  template <typename H>
  void parse_message(
      H&& handler,
      const std::string_view& message) {
    // core::json::Parser parser(message);
    auto type = find_type(message);
    switch (type) {
      case Type::UNKNOWN:
        throw std::runtime_error("Unable to detect message type");
      case Type::RESPONSE:
        // handler(response_t::parse(message));
        break;
      case Type::ERROR:
        // handler(error_t::parse(message));
        break;
      case Type::NOTIFICATION:
        // handler(notification_t::parse(message));
        break;
    }
    /*
    for (auto [key, value] : parser.root<core::json::object_t>()) {
      "jsonrpc"
        "id"
        "testnet"
        "result"
        "error"
        "method"
        "params" --> "channel" + "data"
        "usIn"
        "usOut"
        "usDiff"
    }
    // id -> result|error
    // !id -> method+params
    */
  }

// private:
  enum class Type {
    UNKNOWN,
    RESPONSE,
    ERROR,
    NOTIFICATION
  };
  static Type find_type(const std::string_view& message);

  enum class Field {
    UNKNOWN,
    ERROR,
    ID,
    JSONRPC,
    METHOD,
    PARAMS,
    RESULT,
    TESTNET,
    US_DIFF,
    US_IN,
    US_OUT,
  };
  static constexpr Field parse_name(const std::string_view& name);
};

}  // namespace json
}  // namespace deribit
}  // namespace roq

/*
template <>
struct fmt::formatter<roq::deribit::json::response_t> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::json::response_t& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "best_ask_amount={}, "
        "}}",
        value.best_ask_amount);
  }
};
*/
