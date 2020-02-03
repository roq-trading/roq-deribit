/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct MarketDataRequestReject final {
  std::string_view md_req_id;
  core::fix::MDReqRejReason md_req_rej_reason =
    core::fix::MDReqRejReason::UNKNOWN;
  std::string_view text;

 public:
  MarketDataRequestReject() = default;
  MarketDataRequestReject(MarketDataRequestReject&&) = default;
  MarketDataRequestReject(const MarketDataRequestReject&) = delete;

  static MarketDataRequestReject create(
      const core::fix::message_t& message);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::MarketDataRequestReject> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::MarketDataRequestReject& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "md_req_id=\"{}\", "
        "md_req_rej_reason={}, "
        "text=\"{}\""
        "}}",
        value.md_req_id,
        value.md_req_rej_reason,
        value.text);
  }
};
