/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include "roq/api.h"

namespace roq {
namespace deribit {

struct OrderMapping final {
  OrderMapping(
      uint32_t local_order_id,
      uint8_t user_id,
      uint32_t order_id,
      OrderType order_type,
      Side side,
      const std::string_view& symbol)
      : _local_order_id(local_order_id),
        _user_id(user_id),
        _order_id(order_id),
        _order_type(order_type),
        _side(side) {
    symbol.copy(_symbol, sizeof(_symbol));
  }

  void update_cl_ord_id(
      const std::string_view& cl_ord_id,
      std::chrono::nanoseconds transact_time) {
    cl_ord_id.copy(_cl_ord_id, sizeof(_cl_ord_id));
    _transact_time = transact_time;
  }

  bool ready() const {
    return _transact_time.count() > 0;
  }

 public:
  uint32_t _local_order_id;
  uint8_t _user_id;
  uint32_t _order_id;
  OrderType _order_type;
  Side _side;
  char _symbol[32];
  char _cl_ord_id[32] = {};
  std::chrono::nanoseconds _transact_time = {};
};

}  // namespace deribit
}  // namespace roq
