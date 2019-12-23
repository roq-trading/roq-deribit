/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/resend_request.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/resend_request.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

ResendRequest ResendRequest::parse(const core::fix::message_t& message) {
  ResendRequest result;
  parse(result, message);
  return result;
}

void ResendRequest::parse(
    ResendRequest& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::ResendRequest::has_field(field);
}
}  // namespace

void ResendRequest::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::BEGIN_SEQ_NO:
          static_assert(has_field(core::fix::Field::BEGIN_SEQ_NO));
          core::fix::update(begin_seq_no, value);
          break;
        case core::fix::Field::END_SEQ_NO:
          static_assert(has_field(core::fix::Field::END_SEQ_NO));
          core::fix::update(end_seq_no, value);
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

}  // namespace fix
}  // namespace deribit
}  // namespace roq
