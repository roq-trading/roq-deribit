/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/user_response.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/user_response.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

UserResponse UserResponse::parse(const core::fix::message_t& message) {
  UserResponse result;
  parse(result, message);
  return result;
}

void UserResponse::parse(
    UserResponse& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void UserResponse::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::USERNAME:
          static_assert(core::fix::UserResponse::has_field(core::fix::Field::USERNAME));
          core::fix::update(username, value);
          break;
        case core::fix::Field::USER_REQUEST_ID:
          static_assert(core::fix::UserResponse::has_field(core::fix::Field::USER_REQUEST_ID));
          core::fix::update(user_request_id, value);
          break;
        case core::fix::Field::USER_STATUS:
          static_assert(core::fix::UserResponse::has_field(core::fix::Field::USER_STATUS));
          core::fix::update(user_status, value);
          break;
        // non-standard
        case core::fix::Field::CURRENCY:
          static_assert(!core::fix::UserResponse::has_field(core::fix::Field::CURRENCY));
          core::fix::update(currency, value);
          break;
        default:
          if (core::fix::UserResponse::has_field(field))
            break;
          switch (static_cast<Deribit>(tag)) {
            case Deribit::MARGIN_BALANCE:
              core::fix::update(deribit_margin_balance, value);
              break;
            case Deribit::REALIZED_PL:
              core::fix::update(deribit_realized_pl, value);
              break;
            case Deribit::TOTAL_PL:
              core::fix::update(deribit_total_pl, value);
              break;
            case Deribit::UNREALIZED_PL:
              core::fix::update(deribit_unrealized_pl, value);
              break;
            case Deribit::USER_BALANCE:
              core::fix::update(deribit_user_balance, value);
              break;
            case Deribit::USER_EQUITY:
              core::fix::update(deribit_user_equity, value);
              break;
            case Deribit::USER_INITIAL_MARGIN:
              core::fix::update(deribit_user_initial_margin, value);
              break;
            case Deribit::USER_MAINTENANCE_MARGIN:
              core::fix::update(deribit_user_maintenance_margin, value);
              break;
            default:
              throw core::fix::InvalidField(
                  "UserResponse: "
                  "Unexpected field={}", tag);
          }
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(
          "UserResponse: "
          "Parse error: "
          "field={}, value=\"{}\", what=\"{}\"",
          tag, value, e.what());
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
