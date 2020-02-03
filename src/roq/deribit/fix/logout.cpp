/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/logout.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::Logout::has_field(field);
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
      case core::fix::Field::TEXT:
        check_field<core::fix::Field::TEXT>();
        core::fix::update(result.text, value);
        break;
      default:
        if (has_field(field)) {
          DLOG(FATAL)("Unexpected tag={} field={}", tag, field);
          break;
        }
        DLOG(FATAL)("Unknown tag={} field={}", tag, field);
        throw core::fix::InvalidField(tag, value);
    }
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(tag, value, e);
  }
}
}  // namespace

Logout Logout::create(const core::fix::message_t& message) {
  Logout result;
  for (auto iter = message.begin(); iter != message.end(); ++iter)
    update_field(result, iter);
  return result;
}

core::utils::Message Logout::encode(core::fix::Writer& writer) const {
  return writer
    .write(
        core::fix::Field::TEXT,
        text)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
