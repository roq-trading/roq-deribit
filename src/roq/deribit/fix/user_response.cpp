/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/user_response.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/user_response.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::UserResponse::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

template <auto field>
constexpr void non_standard_field() {
  static_assert(has_field(field) == false);
}

void update_field(
    auto& result,
    auto& iter) {
  auto& [tag, value] = *iter;
  try {
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::USERNAME:
        check_field<core::fix::Field::USERNAME>();
        core::fix::update(result.username, value);
        break;
      case core::fix::Field::USER_REQUEST_ID:
        check_field<core::fix::Field::USER_REQUEST_ID>();
        core::fix::update(result.user_request_id, value);
        break;
      case core::fix::Field::USER_STATUS:
        check_field<core::fix::Field::USER_STATUS>();
        core::fix::update(result.user_status, value);
        break;
      // non-standard
      case core::fix::Field::CURRENCY:
        non_standard_field<core::fix::Field::CURRENCY>();
        core::fix::update(result.currency, value);
        break;
      default:
        if (has_field(field)) {
          DLOG(FATAL)(
              FMT_STRING("Unexpected tag={} field={}"),
              tag,
              field);
          break;
        }
        switch (static_cast<Deribit>(tag)) {
          case Deribit::MARGIN_BALANCE:
            core::fix::update(result.deribit_margin_balance, value);
            break;
          case Deribit::REALIZED_PL:
            core::fix::update(result.deribit_realized_pl, value);
            break;
          case Deribit::TOTAL_PL:
            core::fix::update(result.deribit_total_pl, value);
            break;
          case Deribit::UNREALIZED_PL:
            core::fix::update(result.deribit_unrealized_pl, value);
            break;
          case Deribit::USER_BALANCE:
            core::fix::update(result.deribit_user_balance, value);
            break;
          case Deribit::USER_EQUITY:
            core::fix::update(result.deribit_user_equity, value);
            break;
          case Deribit::USER_INITIAL_MARGIN:
            core::fix::update(result.deribit_user_initial_margin, value);
            break;
          case Deribit::USER_MAINTENANCE_MARGIN:
            core::fix::update(result.deribit_user_maintenance_margin, value);
            break;
          default:
            DLOG(FATAL)(
                FMT_STRING("Unknown tag={} field={}"),
                tag,
                field);
            throw core::fix::InvalidField(tag, value);
        }
    }
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(tag, value, e);
  }
}
}  // namespace

UserResponse UserResponse::create(const core::fix::message_t& message) {
  UserResponse result;
  for (auto iter = message.begin(); iter != message.end(); ++iter)
    update_field(result, iter);
  return result;
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
