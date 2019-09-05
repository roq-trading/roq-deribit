/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/security_list.h"

#include "roq/logging.h"

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
  for (; iter != end;) {
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
        update(security_req_id, value);
        break;
      case core::fix::Field::SECURITY_REQUEST_RESULT:
        update(security_request_result, value);
        break;
      case core::fix::Field::SECURITY_RESPONSE_ID:
        update(security_response_id, value);
        break;
      default:
        LOG(WARNING) << fmt::format(
            "Unknown field: tag={} field={} value=\"{}\"",
            tag,
            field,
            value);
    }
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
