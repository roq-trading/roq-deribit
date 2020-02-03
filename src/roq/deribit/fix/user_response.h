/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <limits>
#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct UserResponse final {
  std::string_view username;
  std::string_view user_request_id;
  core::fix::UserStatus user_status = core::fix::UserStatus::UNKNOWN;
  // non-standard
  std::string_view currency;
  // deribit specific
  double deribit_margin_balance = std::numeric_limits<double>::quiet_NaN();
  double deribit_realized_pl = std::numeric_limits<double>::quiet_NaN();
  double deribit_total_pl = std::numeric_limits<double>::quiet_NaN();
  double deribit_unrealized_pl = std::numeric_limits<double>::quiet_NaN();
  double deribit_user_balance = std::numeric_limits<double>::quiet_NaN();
  double deribit_user_equity = std::numeric_limits<double>::quiet_NaN();
  double deribit_user_initial_margin = std::numeric_limits<double>::quiet_NaN();
  double deribit_user_maintenance_margin = std::numeric_limits<double>::quiet_NaN();

 public:
  UserResponse() = default;
  UserResponse(UserResponse&&) = default;
  UserResponse(const UserResponse&) = delete;

  static UserResponse create(const core::fix::message_t& message);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::UserResponse> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::UserResponse& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "username=\"{}\", "
        "user_request_id=\"{}\", "
        "user_status={}, "
        // non-standard
        "currency=\"{}\", "
        // deribit specific
        "deribit_margin_balance={}, "
        "deribit_realized_pl={}, "
        "deribit_total_pl={}, "
        "deribit_unrealized_pl={}, "
        "deribit_user_balance={}, "
        "deribit_user_equity={}, "
        "deribit_user_initial_margin={}, "
        "deribit_user_maintenance_margin={}"
        "}}",
        value.username,
        value.user_request_id,
        value.user_status,
        // non-standard
        value.currency,
        // deribit specific
        value.deribit_margin_balance,
        value.deribit_realized_pl,
        value.deribit_total_pl,
        value.deribit_unrealized_pl,
        value.deribit_user_balance,
        value.deribit_user_equity,
        value.deribit_user_initial_margin,
        value.deribit_user_maintenance_margin);
  }
};
