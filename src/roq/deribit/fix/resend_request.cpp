/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/resend_request.h"

#include "roq/logging.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

ResendRequest ResendRequest::parse(const core::fix::message_t& message) {
  ResendRequest result;
  parse(result, message);
  return result;
}

void ResendRequest::parse(
    ResendRequest& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void ResendRequest::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::BEGIN_SEQ_NO:
        update(begin_seq_no, value);
        break;
      case core::fix::Field::END_SEQ_NO:
        update(end_seq_no, value);
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
