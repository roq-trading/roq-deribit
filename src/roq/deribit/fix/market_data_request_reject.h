/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct MarketDataRequestReject final {
  std::string_view md_req_id;
  core::fix::MDReqRejReason md_req_rej_reason = core::fix::MDReqRejReason::UNKNOWN;
  std::string_view text;

  static MarketDataRequestReject parse(const core::fix::message_t& message);
  static void parse(MarketDataRequestReject&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);
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
        ctx.begin(),
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
