/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string>

#include "roq/compat.h"

#include "roq/core/utils/message.h"

#include "roq/core/fix/market_data_request.h"
#include "roq/core/fix/writer.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct MarketDataRequest final {
  std::string_view md_req_id;
  roq::span<std::string> symbols;

 public:
  static constexpr auto msg_type = core::fix::MarketDataRequest::msg_type;

  MarketDataRequest() = default;
  MarketDataRequest(MarketDataRequest&&) = default;
  MarketDataRequest(const MarketDataRequest&) = delete;

  core::utils::Message encode(core::fix::Writer& writer) const;
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
    return format_to(
        ctx.out(),
        "{{"
        "md_req_id=\"{}\", "
        "symbols=[{}]"
        "}}",
        value.md_req_id,
        fmt::join(value.symbols, ", "));
  }
};
