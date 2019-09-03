/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/position_report.h"

#include "roq/logging.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

PositionReport PositionReport::parse(
    const core::fix::message_t& message) {
  PositionReport result;
  parse(result, message);
  return result;
}

void PositionReport::parse(
    PositionReport& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void PositionReport::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::NO_POSITIONS:
        // ...
        continue;
      case core::fix::Field::POS_MAINT_RPT_ID:
        update(pos_maint_rpt_id, value);
        break;
      case core::fix::Field::POS_REQ_ID:
        update(pos_req_id, value);
        break;
      case core::fix::Field::POS_REQ_RESULT:
        update(pos_req_result, value);
        break;
      case core::fix::Field::POS_REQ_TYPE:
        update(pos_req_type, value);
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
