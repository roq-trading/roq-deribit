/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include "roq/api.h"

#include "roq/deribit/fix/execution_report.h"

namespace roq {
namespace deribit {

struct OrderMapping final {
  OrderMapping(
      uint32_t local_order_id,
      uint8_t user_id,
      uint32_t order_id,
      OrderType order_type,
      Side side,
      const std::string_view& symbol);

  bool validate(const fix::ExecutionReport& execution_report);

  void update_cl_ord_id(
      const std::string_view& cl_ord_id,
      std::chrono::nanoseconds transact_time);

  bool ready() const {
    return _transact_time.count() > 0;
  }

 public:
  const uint32_t _local_order_id;
  const uint8_t _user_id;
  const uint32_t _order_id;
  const OrderType _order_type;
  const Side _side;
  char _symbol[32] = {};  // note! *not* mutable
  char _cl_ord_id[32] = {};
  std::chrono::nanoseconds _transact_time = {};
};

}  // namespace deribit
}  // namespace roq
