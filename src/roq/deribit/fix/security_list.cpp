/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/security_list.h"

#include "roq/core/charconv.h"

#include "roq/core/fix/array.h"
#include "roq/core/fix/exception.h"
#include "roq/core/fix/reader.h"
#include "roq/core/fix/security_list.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

SecurityList SecurityList::parse(
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  SecurityList result;
  parse(result, message, buffer);
  return result;
}

void SecurityList::parse(
    SecurityList& result,
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end(), buffer);
}

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::SecurityList::has_field(field);
}
}  // namespace

void SecurityList::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end,
    core::fix::Buffer& buffer) {
  while (iter != end) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::NO_RELATED_SYM: {
          static_assert(has_field(core::fix::Field::NO_RELATED_SYM));
          auto length = core::from_chars<uint32_t>(value);
          if (length) {
            ++iter;
            if (iter == end)
              throw core::fix::UnexpectedEndOfMessage();
            core::fix::Array array(buffer, instruments, length);
            for (auto& item : array)
              item.parse(iter, end);
            continue;
          }
          break;
        }
        case core::fix::Field::SECURITY_REQ_ID:
          static_assert(has_field(core::fix::Field::SECURITY_REQ_ID));
          core::fix::update(security_req_id, value);
          break;
        case core::fix::Field::SECURITY_REQUEST_RESULT:
          static_assert(has_field(core::fix::Field::SECURITY_REQUEST_RESULT));
          core::fix::update(security_request_result, value);
          break;
        case core::fix::Field::SECURITY_RESPONSE_ID:
          static_assert(has_field(core::fix::Field::SECURITY_RESPONSE_ID));
          core::fix::update(security_response_id, value);
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
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
