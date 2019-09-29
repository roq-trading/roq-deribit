/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/logout.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/logout.h"
#include "roq/core/fix/writer.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

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

void Logout::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::TEXT:
          static_assert(core::fix::Logout::has_field(core::fix::Field::TEXT));
          core::fix::update(text, value);
          break;
        default:
          if (core::fix::Logout::has_field(field))
            break;
          throw core::fix::InvalidField(
              "Logout: "
              "Unexpected field={}", tag);
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(
          "Logout: "
          "Parse error: "
          "field={}, value=\"{}\", what=\"{}\"",
          tag, value, e.what());
    }
  }
}

core::utils::Message Logout::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time) const {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::Logout::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::TEXT, text)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
