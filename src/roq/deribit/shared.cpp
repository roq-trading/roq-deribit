/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/shared.h"

#include <magic_enum.hpp>

#include "roq/deribit/flags.h"

namespace roq {
namespace deribit {

Shared::Shared(server::Dispatcher &dispatcher)
    : fills(server::Flags::cache_fills_max_depth()), bids(server::Flags::cache_mbp_max_depth()),
      asks(server::Flags::cache_mbp_max_depth()), trades(server::Flags::cache_trades_max_depth()),
      statistics(magic_enum::enum_count<StatisticsType::type_t>()), dispatcher_(dispatcher) {
}

}  // namespace deribit
}  // namespace roq
