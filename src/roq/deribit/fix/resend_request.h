/* Copyright (c) 2017-2020, Hans Erik Thrane */

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

 public:
  ResendRequest() = default;
  ResendRequest(ResendRequest&&) = default;
  ResendRequest(const ResendRequest&) = delete;

  static ResendRequest create(const core::fix::message_t& message);
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
