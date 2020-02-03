/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/core/utils/buffer.h"
#include "roq/core/utils/message.h"

#include "roq/core/fix/logout.h"
#include "roq/core/fix/reader.h"
#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

struct Logout final {
  std::string_view text;

 public:
  static constexpr auto msg_type = core::fix::Logout::msg_type;

  Logout() = default;
  Logout(Logout&&) = default;
  Logout(const Logout&) = delete;

  static Logout create(const core::fix::message_t& message);

  core::utils::Message encode(core::fix::Writer& writer) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::Logout> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::Logout& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "text=\"{}\""
        "}}",
        value.text);
  }
};
