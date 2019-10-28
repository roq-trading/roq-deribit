/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include "roq/api.h"

#include "roq/core/oms/user_custom.h"
#include "roq/deribit/fix/execution_report.h"

namespace roq {
namespace deribit {

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
  const core::oms::UserCustom _user_custom;
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
