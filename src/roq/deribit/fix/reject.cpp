/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/reject.h"

#include "roq/logging.h"

#include "roq/core/fix/reject.h"
#include "roq/core/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

Reject Reject::parse(
    const core::fix::message_t& message) {
  Reject result;
  parse(result, message);
  return result;
}

void Reject::parse(
    Reject& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void Reject::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::REF_SEQ_NUM:
          static_assert(core::fix::Reject::has_field(core::fix::Field::REF_SEQ_NUM));
          core::fix::update(ref_seq_num, value);
          break;
        case core::fix::Field::REF_TAG_ID:
          static_assert(core::fix::Reject::has_field(core::fix::Field::REF_TAG_ID));
          core::fix::update(ref_tag_id, value);
          break;
        case core::fix::Field::REF_MSG_TYPE:
          static_assert(core::fix::Reject::has_field(core::fix::Field::REF_MSG_TYPE));
          core::fix::update(ref_msg_type, value);
          break;
        case core::fix::Field::TEXT:
          static_assert(core::fix::Reject::has_field(core::fix::Field::TEXT));
          core::fix::update(text, value);
          break;
        default:
          if (core::fix::Reject::has_field(field))
            break;
      }
    } catch (std::exception& e) {
      LOG(WARNING) << fmt::format(
          "Can't parse tag={} value=\"{}\"", tag, value);
      throw;
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
