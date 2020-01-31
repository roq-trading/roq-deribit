/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/logon.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

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

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::Logon::has_field(field);
}
}  // namespace

void Logon::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::HEART_BT_INT:
          static_assert(has_field(core::fix::Field::HEART_BT_INT));
          core::fix::update(heart_bt_int, value);
          break;
        case core::fix::Field::RAW_DATA_LENGTH:
          static_assert(has_field(core::fix::Field::RAW_DATA_LENGTH));
          // not needed
          break;
        case core::fix::Field::RAW_DATA:
          static_assert(has_field(core::fix::Field::RAW_DATA));
          core::fix::update(raw_data, value);
          break;
        case core::fix::Field::USERNAME:
          static_assert(has_field(core::fix::Field::USERNAME));
          core::fix::update(username, value);
          break;
        case core::fix::Field::PASSWORD:
          static_assert(has_field(core::fix::Field::PASSWORD));
          core::fix::update(password, value);
          break;
        default:
          if (has_field(field)) {
            DLOG(FATAL)("Unexpected tag={} field={}", tag, field);
            break;
          }
          switch (static_cast<Deribit>(tag)) {
            case Deribit::CANCEL_ON_DISCONNECT:
              core::fix::update(deribit_cancel_on_disconnect, value);
              break;
            case Deribit::USE_WORDSAFE_TAGS:
              core::fix::update(deribit_use_wordsafe_tags, value);
              break;
            default:
              DLOG(FATAL)("Unknown tag={} field={}", tag, field);
              throw core::fix::InvalidField(tag, value);
          }
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(tag, value, e);
    }
  }
}

core::utils::Message Logon::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(core::fix::Field::HEART_BT_INT, heart_bt_int)
    .write(core::fix::Field::RAW_DATA, raw_data)
    .write(core::fix::Field::USERNAME, username)
    .write(core::fix::Field::PASSWORD, password)
    .write(
        static_cast<uint32_t>(fix::Deribit::CANCEL_ON_DISCONNECT),
        deribit_cancel_on_disconnect)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
