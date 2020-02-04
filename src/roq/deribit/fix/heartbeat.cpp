/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/heartbeat.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::Heartbeat::has_field(field);
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
      case core::fix::Field::TEST_REQ_ID:
        check_field<core::fix::Field::TEST_REQ_ID>();
        core::fix::update(result.test_req_id, value);
        break;
      default:
        if (has_field(field)) {
          DLOG(FATAL)(
              FMT_STRING("Unexpected tag={} field={}"),
              tag,
              field);
          break;
        }
        DLOG(FATAL)(
          FMT_STRING("Unknown tag={} field={}"),
          tag,
          field);
        throw core::fix::InvalidField(tag, value);
    }
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(tag, value, e);
  }
}
}  // namespace

Heartbeat Heartbeat::create(const core::fix::message_t& message) {
  Heartbeat result;
  for (auto iter = message.begin(); iter != message.end(); ++iter)
    update_field(result, iter);
  return result;
}

core::utils::Message Heartbeat::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(
        core::fix::Field::TEST_REQ_ID,
        test_req_id)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
