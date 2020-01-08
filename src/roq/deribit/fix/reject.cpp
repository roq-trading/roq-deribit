/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/reject.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"

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

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::Reject::has_field(field);
}
}  // namespace

void Reject::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::REF_SEQ_NUM:
          static_assert(has_field(core::fix::Field::REF_SEQ_NUM));
          core::fix::update(ref_seq_num, value);
          break;
        case core::fix::Field::REF_TAG_ID:
          static_assert(has_field(core::fix::Field::REF_TAG_ID));
          core::fix::update(ref_tag_id, value);
          break;
        case core::fix::Field::REF_MSG_TYPE:
          static_assert(has_field(core::fix::Field::REF_MSG_TYPE));
          core::fix::update(ref_msg_type, value);
          break;
        case core::fix::Field::TEXT:
          static_assert(has_field(core::fix::Field::TEXT));
          core::fix::update(text, value);
          break;
        default:
          if (has_field(field))
            break;
          throw core::fix::InvalidField(tag, value);
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(tag, value, e);
    }
  }
}

core::utils::Message Reject::encode(core::fix::Writer& writer) const {
  return writer
    .write(core::fix::Field::REF_SEQ_NUM, ref_seq_num)
    .write(core::fix::Field::REF_MSG_TYPE, ref_msg_type)
    .write(core::fix::Field::TEXT, text)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
