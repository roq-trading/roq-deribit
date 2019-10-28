/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/order_mapping.h"

#include "roq/logging.h"

#include "roq/core/fix/utils.h"

namespace roq {
namespace deribit {

OrderMapping::OrderMapping(
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
  auto length = symbol.copy(_symbol, sizeof(_symbol));
  if (length < sizeof(_symbol))
    _symbol[length] = '\0';
}

bool OrderMapping::validate(
    const fix::ExecutionReport& execution_report) {
  bool result = true;
  // symbol
  if (unlikely(
        execution_report.symbol.empty() == false &&
        execution_report.symbol.compare(_symbol) != 0)) {
    LOG(WARNING)(
        "Unexpected symbol, received \"{}\", expected \"{}\"",
        execution_report.symbol,
        _symbol);
    result = false;
  }
  // side
  auto side = core::fix::map(execution_report.side);
  if (unlikely(side != Side::UNDEFINED && side != _side)) {
    LOG(WARNING)(
        "Unexpected side, received {}, expected {}",
        side,
        _side);
    result = false;
  }
  // cl_ord_id
  if (_cl_ord_id[0] == '\0') {
    if (execution_report.cl_ord_id.empty()) {
      LOG(WARNING)("Missing cl_ord_id");
      result = false;
    } else {
      auto length = execution_report.cl_ord_id.copy(
          _cl_ord_id,
          sizeof(_cl_ord_id));
      if (length < sizeof(_cl_ord_id))
        _cl_ord_id[length] = '\0';
    }
  } else {
    if (execution_report.cl_ord_id.compare(_cl_ord_id) != 0) {
      LOG(WARNING)(
          "Unexpected cl_ord_id, received \"{}\", expected \"{}\"",
          execution_report.cl_ord_id,
          _cl_ord_id);
      result = false;
    }
  }
  // transact_time
  if (_transact_time.count() == 0)
    _transact_time = execution_report.transact_time;
  return result;
}

void OrderMapping::update_cl_ord_id(
    const std::string_view& cl_ord_id,
    std::chrono::nanoseconds transact_time) {
  auto length = cl_ord_id.copy(_cl_ord_id, sizeof(_cl_ord_id));
  if (length < sizeof(_cl_ord_id))
    _cl_ord_id[length] = '\0';
  _transact_time = transact_time;
}

void OrderMapping::update_request(
    uint32_t request_id,
    Request request) {
  assert(_request_id == uint32_t{0});
  assert(_request == Request::NONE);
  _request_id = request_id;
  _request = request;
}

void OrderMapping::reset_request() {
  assert(_request_id > uint32_t{0});
  assert(_request != Request::NONE);
  _request_id = uint32_t{0};
  _request = Request::NONE;
}

}  // namespace deribit
}  // namespace roq
