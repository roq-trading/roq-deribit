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

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::Logon::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

void update_field(
    auto& result,
    auto& iter) {
  auto& [tag, value] = *iter;
  try {
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::HEART_BT_INT:
        check_field<core::fix::Field::HEART_BT_INT>();
        core::fix::update(result.heart_bt_int, value);
        break;
      case core::fix::Field::RAW_DATA_LENGTH:
        check_field<core::fix::Field::RAW_DATA_LENGTH>();
        // not needed
        break;
      case core::fix::Field::RAW_DATA:
        check_field<core::fix::Field::RAW_DATA>();
        core::fix::update(result.raw_data, value);
        break;
      case core::fix::Field::USERNAME:
        check_field<core::fix::Field::USERNAME>();
        core::fix::update(result.username, value);
        break;
      case core::fix::Field::PASSWORD:
        check_field<core::fix::Field::PASSWORD>();
        core::fix::update(result.password, value);
        break;
      default:
        if (has_field(field)) {
          DLOG(FATAL)(
              FMT_STRING("Unexpected tag={} field={}"),
              tag,
              field);
          break;
        }
        switch (static_cast<Deribit>(tag)) {
          case Deribit::CANCEL_ON_DISCONNECT:
            core::fix::update(result.deribit_cancel_on_disconnect, value);
            break;
          case Deribit::USE_WORDSAFE_TAGS:
            core::fix::update(result.deribit_use_wordsafe_tags, value);
            break;
          default:
            DLOG(FATAL)(
                FMT_STRING("Unknown tag={} field={}"),
                tag,
                field);
            throw core::fix::InvalidField(tag, value);
        }
    }
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(tag, value, e);
  }
}
}  // namespace

Logon Logon::create(const core::fix::message_t& message) {
  Logon result;
  for (auto iter = message.begin(); iter != message.end(); ++iter)
    update_field(result, iter);
  return result;
}

core::utils::Message Logon::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(
        core::fix::Field::HEART_BT_INT,
        heart_bt_int)
    .write(
        core::fix::Field::RAW_DATA,
        raw_data)
    .write(
        core::fix::Field::USERNAME,
        username)
    .write(
        core::fix::Field::PASSWORD,
        password)
    .write(
        static_cast<uint32_t>(fix::Deribit::CANCEL_ON_DISCONNECT),
        deribit_cancel_on_disconnect)
    .write(
        static_cast<uint32_t>(fix::Deribit::USE_WORDSAFE_TAGS),
        deribit_use_wordsafe_tags)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
