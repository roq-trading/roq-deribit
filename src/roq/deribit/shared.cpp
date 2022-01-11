/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/shared.h"

#include "roq/deribit/flags/common.h"
#include "roq/deribit/flags/fix.h"

using namespace std::literals;

namespace roq {
namespace deribit {

Shared::Shared(server::Dispatcher &dispatcher)
    : fills(server::Flags::cache_fills_max_depth()), bids(server::Flags::cache_mbp_max_depth()),
      asks(server::Flags::cache_mbp_max_depth()), final_bids(server::Flags::cache_mbp_max_depth()),
      final_asks(server::Flags::cache_mbp_max_depth()),
      trades(server::Flags::cache_trades_max_depth()), statistics(StatisticsType::count()),
      dispatcher_(dispatcher),
      rate_limiter(flags::Common::request_limit(), flags::Common::request_limit_interval()),
      symbols(flags::FIX::fix_market_data_max_subscriptions_per_stream()) {
}

std::string_view Shared::next_request_id() {
  auto request_id = ++request_id_;
  stack_buffer_.clear();
  fmt::format_to(std::back_inserter(stack_buffer_), "roq-{}"sv, request_id);
  return std::string_view{std::data(stack_buffer_), std::size(stack_buffer_)};
}

}  // namespace deribit
}  // namespace roq
