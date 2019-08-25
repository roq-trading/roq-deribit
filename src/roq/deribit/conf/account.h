/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <ostream>
#include <string>

#include "roq/server.h"

namespace roq {
namespace deribit {
namespace conf {

struct Account final {
  roq::Account account;
  std::string broker;
  int seat_no;
  bool master;
};

std::ostream& operator<<(
    std::ostream& stream,
    const Account& value);

}  // namespace conf
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::conf::Account> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::conf::Account& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "account={}, "
        "broker=\"{}\", "
        "seat_no={}, "
        "master={}"
        "}}",
        value.account,
        value.broker,
        value.seat_no,
        value.master);
  }
};
