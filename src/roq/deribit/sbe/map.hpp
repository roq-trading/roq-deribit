/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/map.hpp"

#include "roq/liquidity.hpp"
#include "roq/side.hpp"
#include "roq/trading_status.hpp"

#include <deribit_multicast/BookSide.h>
#include <deribit_multicast/Direction.h>
#include <deribit_multicast/InstrumentState.h>
#include <deribit_multicast/Liquidation.h>
#include <deribit_multicast/YesNo.h>

namespace roq {

template <>
template <>
std::optional<Side> Map<deribit_multicast::BookSide::Value>::helper() const;

template <>
template <>
std::optional<Side> Map<deribit_multicast::Direction::Value>::helper() const;

template <>
template <>
std::optional<TradingStatus> Map<deribit_multicast::InstrumentState::Value>::helper() const;

template <>
template <>
std::optional<Liquidity> Map<deribit_multicast::Liquidation::Value>::helper() const;

template <>
template <>
std::optional<bool> Map<deribit_multicast::YesNo::Value>::helper() const;

}  // namespace roq
