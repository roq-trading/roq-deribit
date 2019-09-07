/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/heartbeat.h"

#include "roq/logging.h"

#include "roq/core/fix/heartbeat.h"

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
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::TEST_REQ_ID:
          update(test_req_id, value);
          break;
        default:
          if (core::fix::Heartbeat::has_field(field))
            break;
          throw std::runtime_error(
              fmt::format(
                  "Unknown field: tag={} field={} value=\"{}\"",
                  tag, field, value));
      }
    } catch (std::exception& e) {
      LOG(WARNING) <<
        fmt::format("Can't parse tag={} value=\"{}\"", tag, value);
      throw;
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
