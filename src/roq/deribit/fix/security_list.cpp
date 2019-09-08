/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/security_list.h"

#include <fmt/format.h>

#include <stdexcept>

#include "roq/core/fix/security_list.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/array.h"
#include "roq/deribit/fix/buffer.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

SecurityList SecurityList::parse(
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  SecurityList result;
  parse(result, message, buffer);
  return result;
}

void SecurityList::parse(
    SecurityList& result,
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end(), buffer);
}

void SecurityList::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end,
    std::vector<std::byte>& buffer) {
  Buffer buffer_(buffer);
  while (iter != end) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::NO_RELATED_SYM: {
        auto length = core::charconv::from_string<uint32_t>(value);
        ++iter;
        Array array(buffer_, instruments);
        for (uint32_t i = 0; i < length; ++i) {
          auto& item = array.next();
          item.parse(iter, end);
          ++array;
        }
        continue;
      }
      case core::fix::Field::SECURITY_REQ_ID:
        core::fix::update(security_req_id, value);
        break;
      case core::fix::Field::SECURITY_REQUEST_RESULT:
        core::fix::update(security_request_result, value);
        break;
      case core::fix::Field::SECURITY_RESPONSE_ID:
        core::fix::update(security_response_id, value);
        break;
      default:
        if (core::fix::SecurityList::has_field(field))
          break;
        throw std::runtime_error(
            fmt::format(
                "Unknown field: tag={} field={} value=\"{}\"",
                tag, field, value));
    }
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
