/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/logout.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

Logout Logout::parse(const core::fix::message_t& message) {
  Logout result;
  parse(result, message);
  return result;
}

void Logout::parse(
    Logout& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::Logout::has_field(field);
}
}  // namespace

void Logout::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::TEXT:
          static_assert(has_field(core::fix::Field::TEXT));
          core::fix::update(text, value);
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
}

core::utils::Message Logout::encode(core::fix::Writer& writer) const {
  return writer
    .write(core::fix::Field::TEXT, text)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
