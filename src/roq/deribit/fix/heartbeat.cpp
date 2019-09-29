/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/heartbeat.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/heartbeat.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

Heartbeat Heartbeat::parse(const core::fix::message_t& message) {
  Heartbeat result;
  parse(result, message);
  return result;
}

void Heartbeat::parse(
    Heartbeat& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void Heartbeat::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::TEST_REQ_ID:
          static_assert(core::fix::Heartbeat::has_field(core::fix::Field::TEST_REQ_ID));
          core::fix::update(test_req_id, value);
          break;
        default:
          if (core::fix::Heartbeat::has_field(field))
            break;
          throw core::fix::InvalidField(
              "Heartbeat: "
              "Unexpected field={}", tag);
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(
          "Heartbeat: "
          "Parse error: "
          "field={}, value=\"{}\", what=\"{}\"",
          tag, value, e.what());
    }
  }
}

core::utils::Message Heartbeat::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time) const {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::Heartbeat::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::TEST_REQ_ID, test_req_id)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
