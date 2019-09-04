/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/security_list.h"

#include "roq/logging.h"

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
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::NO_RELATED_SYM:
        // update(security_request_result, value);
        continue;
      case core::fix::Field::SECURITY_REQ_ID:
        update(security_req_id, value);
        break;
      case core::fix::Field::SECURITY_REQUEST_RESULT:
        update(security_request_result, value);
        break;
      case core::fix::Field::SECURITY_RESPONSE_ID:
        update(security_response_id, value);
        break;
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
