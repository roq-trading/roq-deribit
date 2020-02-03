/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/security_list.h"

#include "roq/core/charconv.h"

#include "roq/core/fix/array.h"
#include "roq/core/fix/exception.h"
#include "roq/core/fix/reader.h"
#include "roq/core/fix/security_list.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::SecurityList::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

template <auto field>
constexpr void non_standard_field() {
  static_assert(has_field(field) == false);
}

void update(
    auto& result,
    auto&& iter,
    const auto& end,
    auto& buffer) {
  while (iter != end) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::NO_RELATED_SYM: {
          check_field<core::fix::Field::NO_RELATED_SYM>();
          result.instruments =
            core::fix::Array<decltype(result.instruments)>::create(
                buffer,
                iter,
                end);
          continue;  // note!
        }
        case core::fix::Field::SECURITY_REQ_ID:
          check_field<core::fix::Field::SECURITY_REQ_ID>();
          core::fix::update(result.security_req_id, value);
          break;
        case core::fix::Field::SECURITY_REQUEST_RESULT:
          check_field<core::fix::Field::SECURITY_REQUEST_RESULT>();
          core::fix::update(result.security_request_result, value);
          break;
        case core::fix::Field::SECURITY_RESPONSE_ID:
          check_field<core::fix::Field::SECURITY_RESPONSE_ID>();
          core::fix::update(result.security_response_id, value);
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
    ++iter;
  }
}
}  // namespace

SecurityList SecurityList::create(
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  SecurityList result;
  update(
      result,
      message.begin(),
      message.end(),
      buffer);
  return result;
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
