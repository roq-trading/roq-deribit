/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/logon.h"

#include "roq/logging.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

Logon Logon::parse(
    const core::fix::message_t& message) {
  Logon result;
  parse(result, message);
  return result;
}

void Logon::parse(
    Logon& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void Logon::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::HEART_BT_INT:
        update(heart_bt_int, value);
        break;
      case core::fix::Field::RAW_DATA_LENGTH:
        // not needed
        break;
      case core::fix::Field::RAW_DATA:
        update(raw_data, value);
        break;
      case core::fix::Field::USERNAME:
        update(username, value);
        break;
      case core::fix::Field::PASSWORD:
        update(password, value);
        break;
      default:
        switch (static_cast<Deribit>(tag)) {
          case Deribit::CANCEL_ON_DISCONNECT:
            update(cancel_on_disconnect, value);
            break;
          case Deribit::USE_WORDSAFE_TAGS:
            update(use_wordsafe_tags, value);
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
