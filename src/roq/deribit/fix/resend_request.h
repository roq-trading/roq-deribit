/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct ResendRequest final {
  uint64_t begin_seq_no = 0;
  uint64_t end_seq_no = 0;

  static ResendRequest parse(const core::fix::message_t& message);
  static void parse(ResendRequest&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::ResendRequest> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::ResendRequest& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "begin_seq_no={}, "
        "end_seq_no={}"
        "}}",
        value.begin_seq_no,
        value.end_seq_no);
  }
};
