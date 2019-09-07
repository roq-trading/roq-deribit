/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct Logon final {
  uint16_t heart_bt_int = 0;
  std::string_view raw_data;
  std::string_view username;
  std::string_view password;
  // deribit specific
  bool deribit_cancel_on_disconnect = false;
  bool deribit_use_wordsafe_tags = false;

  static Logon parse(const core::fix::message_t& message);
  static void parse(Logon&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::Logon> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::Logon& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "heart_bt_int={}, "
        "raw_data=\"{}\", "
        "username=\"{}\", "
        "password=\"{}\", "
        "deribit_cancel_on_disconnect={}, "
        "deribit_use_wordsafe_tags={}"
        "}}",
        value.heart_bt_int,
        value.raw_data,
        value.username,
        value.password,
        value.deribit_cancel_on_disconnect,
        value.deribit_use_wordsafe_tags);
  }
};
