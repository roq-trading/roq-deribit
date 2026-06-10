/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/deribit/protocol/json/direction.hpp"
#include "roq/deribit/protocol/json/liquidity.hpp"
#include "roq/deribit/protocol/json/state.hpp"

#include "roq/liquidity.hpp"
#include "roq/side.hpp"
#include "roq/trading_status.hpp"

#include "roq/map.hpp"

namespace roq {

template <>
template <>
std::optional<Side> Map<deribit::protocol::json::Direction>::helper() const;

template <>
template <>
std::optional<Liquidity> Map<deribit::protocol::json::Liquidity>::helper() const;

template <>
template <>
std::optional<TradingStatus> Map<deribit::protocol::json::State>::helper() const;

}  // namespace roq
