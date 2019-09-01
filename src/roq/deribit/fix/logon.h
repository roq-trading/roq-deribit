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
  bool cancel_on_disconnect = false;
  bool use_wordsafe_tags = false;

  static void parse(Logon&, const core::fix::header_t&, const core::fix::body_t&);
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
        "cancel_on_disconnect={}, "
        "use_wordsafe_tag={}"
        "}}",
        value.heart_bt_int,
        value.raw_data,
        value.username,
        value.password,
        value.cancel_on_disconnect,
        value.use_wordsafe_tags);
  }
};
