/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/order_mapping.h"

#include <fmt/format.h>

#include "roq/logging.h"

#include "roq/core/charconv/number.h"

#include "roq/core/fix/utils.h"

namespace roq {
namespace deribit {

namespace {
template <typename T>
inline void copy_to(const std::string_view& value, T& result) {
  result[value.copy(result, sizeof(result) - 1)] = '\0';
}
}  // namespace

UserCustom::UserCustom(
    uint8_t user_id,
    uint32_t user_order_id,
    uint32_t gateway_order_id)
    : _user_id(user_id),
      _user_order_id(user_order_id),
      _gateway_order_id(gateway_order_id) {
  auto key = (static_cast<uint64_t>(user_id) << 32) |
    static_cast<uint64_t>(user_order_id);
  auto text = fmt::format("ROQ:{}-{}", key, gateway_order_id);
  copy_to(text, _text);
}

UserCustom::UserCustom(const std::string_view& text) {
  if (text.size() > 4 &&
      text.compare(0, 4, "ROQ:") == 0 &&
      text.size() < 32) {
    auto iter = text.begin() + 4,
         end = text.end();
    auto key = core::charconv::parse_number<uint64_t>(iter, end);
    if (iter != end && (*iter) == '-') {
      ++iter;
      _gateway_order_id = core::charconv::parse_number<uint32_t>(
          iter, end);
      if (iter == end) {
        _user_id = static_cast<uint8_t>(key >> 32);
        _user_order_id = static_cast<uint32_t>(key);
        copy_to(text, _text);
        return;
      }
    }
  }
  throw InvalidUserCustom("Invalid UserCustom");
}

OrderMapping::OrderMapping(
    const MessageInfo& message_info,
    const CreateOrder& create_order,
    uint32_t gateway_order_id)
    : _user_custom(
        message_info.source,
        create_order.order_id,
        gateway_order_id),
      _order_type(create_order.order_type),
      _side(create_order.side) {
  copy_to(create_order.symbol, _symbol);
}

OrderMapping::OrderMapping(
    const fix::ExecutionReport& execution_report,
    const std::string_view& user_custom)
    : _user_custom(user_custom),
      _order_type(core::fix::map(execution_report.ord_type)),
      _side(core::fix::map(execution_report.side)) {
  copy_to(execution_report.symbol, _symbol);
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
  // exchange_order_id
  if (_exchange_order_id[0] == '\0') {
    if (execution_report.cl_ord_id.empty()) {
      LOG(WARNING)("Missing cl_ord_id");
      result = false;
    } else {
      copy_to(execution_report.cl_ord_id, _exchange_order_id);
    }
  } else {
    if (execution_report.cl_ord_id.compare(_exchange_order_id) != 0) {
      LOG(WARNING)(
          "Unexpected cl_ord_id, received \"{}\", expected \"{}\"",
          execution_report.cl_ord_id,
          _exchange_order_id);
      result = false;
    }
  }
  // transact_time
  if (_create_time.count() == 0)
    _create_time = execution_report.transact_time;
  _update_time = execution_report.transact_time;
  return result;
}

// ...

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
