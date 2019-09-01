/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct Heartbeat final {
  static void parse(Heartbeat&, const core::fix::header_t&, const core::fix::body_t&);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::Heartbeat> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::Heartbeat& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "}}");
  }
};
