/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/deribit/json/direction.hpp"
#include "roq/deribit/json/liquidity.hpp"
#include "roq/deribit/json/state.hpp"

#include "roq/liquidity.hpp"
#include "roq/side.hpp"
#include "roq/trading_status.hpp"

#include "roq/map.hpp"

namespace roq {

template <>
template <>
std::optional<Side> Map<deribit::json::Direction>::helper() const;

template <>
template <>
std::optional<Liquidity> Map<deribit::json::Liquidity>::helper() const;

template <>
template <>
std::optional<TradingStatus> Map<deribit::json::State>::helper() const;

}  // namespace roq
