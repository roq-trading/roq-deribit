/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/core/utils/buffer.h"
#include "roq/core/utils/message.h"

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct Logout final {
  std::string_view text;

  static Logout parse(const core::fix::message_t& message);
  static void parse(Logout&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);

  static core::utils::Message encode(
      core::utils::Buffer& buffer,
      uint64_t& msg_seq_num,
      std::chrono::nanoseconds sending_time,
      const std::string_view& test_req_id);
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
