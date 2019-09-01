/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/logon.h"

#include "roq/logging.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

void Logon::parse(
    Logon& result,
    const core::fix::header_t&,
    const core::fix::body_t& object) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  for (auto [tag, value] : object) {
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::HEART_BT_INT:
        update(result.heart_bt_int, value);
        break;
      case core::fix::Field::RAW_DATA_LENGTH:
        // not needed
        break;
      case core::fix::Field::RAW_DATA:
        update(result.raw_data, value);
        break;
      case core::fix::Field::USERNAME:
        update(result.username, value);
        break;
      case core::fix::Field::PASSWORD:
        update(result.password, value);
        break;
      default:
        switch (tag) {
          case 9001:
            update(result.cancel_on_disconnect, value);
            break;
          case 9002:
            update(result.use_wordsafe_tags, value);
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
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
