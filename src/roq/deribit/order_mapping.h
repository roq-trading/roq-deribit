/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include "roq/api.h"

#include "roq/deribit/fix/execution_report.h"

namespace roq {
namespace deribit {

class InvalidUserCustom final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

struct UserCustom final {
  UserCustom(
      uint8_t user_id,
      uint32_t user_order_id,
      uint32_t gateway_order_id);

  explicit UserCustom(const std::string_view& text);

  inline auto user_id() const {
    return _user_id;
  }
  inline auto user_order_id() const {
    return _user_order_id;
  }
  inline auto gateway_order_id() const {
    return _gateway_order_id;
  }

  inline std::string_view text() const {
    return _text;
  }

  inline operator std::string_view() const {
    return text();
  }

  inline auto key() const {
    return (static_cast<uint64_t>(_user_id) << 32) |
      static_cast<uint64_t>(_user_order_id);
  }

 private:
  uint8_t _user_id = 0;
  uint32_t _user_order_id = 0;
  uint32_t _gateway_order_id = 0;
  char _text[32] = {};
};

struct OrderMapping final {
  OrderMapping(
      const MessageInfo& message_info,
      const CreateOrder& create_order,
      uint32_t gateway_order_id);

  OrderMapping(
      const fix::ExecutionReport& execution_report,
      const std::string_view& user_custom);

  inline const auto& user_custom() const {
    return _user_custom;
  }

  inline auto user_id() const {
    return _user_custom.user_id();
  }
  inline auto user_order_id() const {
    return _user_custom.user_order_id();
  }
  inline auto gateway_order_id() const {
    return _user_custom.gateway_order_id();
  }

  inline auto key() const {
    return _user_custom.key();
  }

  inline auto order_type() const {
    return _order_type;
  }
  inline auto side() const {
    return _side;
  }

  inline std::string_view symbol() const {
    return _symbol;
  }
  inline std::string_view exchange_order_id() const {
    return _exchange_order_id;
  }

  inline auto create_time() const {
    return _create_time;
  }
  inline auto update_time() const {
    return _update_time;
  }

  bool validate(const fix::ExecutionReport& execution_report);

  inline bool ready() const {
    return _create_time.count() > 0;
  }

  // ...

  enum class Request {
    NONE,
    CREATE,
    MODIFY,
    CANCEL,
  };

  auto request() const {
    return _request;
  }

  void update_request(
      uint32_t request_id,
      Request request);

  void reset_request();

 private:
  const UserCustom _user_custom;
  const OrderType _order_type;
  const Side _side;
  char _symbol[32] = {};  // note! *not* mutable
  char _exchange_order_id[32] = {};
  std::chrono::nanoseconds _create_time = {};
  std::chrono::nanoseconds _update_time = {};
  // request
  uint32_t _request_id = 0;
  Request _request = Request::NONE;
};

}  // namespace deribit
}  // namespace roq
