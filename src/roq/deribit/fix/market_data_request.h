/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>
#include <vector>

#include "roq/core/utils/message.h"

#include "roq/core/fix/order_cancel_request.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct MarketDataRequest final {
  std::string_view md_req_id;
  // optional -- single or list
  std::string_view symbol;
  std::vector<std::string_view> symbols;

  core::utils::Message encode(
      core::utils::Buffer& buffer,
      uint64_t& msg_seq_num,
      std::chrono::nanoseconds sending_time) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::MarketDataRequest> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::MarketDataRequest& value, C& ctx) {
    if (value.symbols.empty()) {
      return format_to(
          ctx.out(),
          "{{"
          "md_req_id=\"{}\", "
          "symbol=\"{}\""
          "}}",
          value.md_req_id,
          value.symbol);
    }
    return format_to(
        ctx.out(),
        "{{"
        "md_req_id=\"{}\", "
        "symbol=[\"{}\"]"
        "}}",
        value.md_req_id,
        fmt::join(value.symbols, "\", \""));
  }
};
