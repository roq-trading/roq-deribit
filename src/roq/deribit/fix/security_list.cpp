/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/security_list.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

void SecurityList::parse(
    SecurityList& result,
    const core::fix::header_t&,
    const core::fix::body_t& object) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  for (auto [tag, value] : object) {
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::TEXT:
        result.text = value;
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
