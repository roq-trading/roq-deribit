/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/heartbeat.h"

#include "roq/logging.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

Heartbeat Heartbeat::parse(const core::fix::message_t& message) {
  Heartbeat result;
  parse(result, message);
  return result;
}

void Heartbeat::parse(
    Heartbeat& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void Heartbeat::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::TEST_REQ_ID:
        update(test_req_id, value);
        break;
      default:
        LOG(WARNING) << fmt::format(
            "Unknown field: tag={} field={} value=\"{}\"",
            tag,
            field,
            value);
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
