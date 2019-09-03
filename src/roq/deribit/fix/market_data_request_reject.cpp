/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_request_reject.h"

#include "roq/logging.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

MarketDataRequestReject MarketDataRequestReject::parse(
    const core::fix::message_t& message) {
  MarketDataRequestReject result;
  parse(result, message);
  return result;
}
void MarketDataRequestReject::parse(
    MarketDataRequestReject& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void MarketDataRequestReject::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::MD_REQ_ID:
        update(md_req_id, value);
        break;
      case core::fix::Field::MD_REQ_REJ_REASON:
        update(md_req_rej_reason, value);
        break;
      case core::fix::Field::TEXT:
        update(text, value);
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
