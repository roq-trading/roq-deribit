/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <deribit/sbe/multicast/BookSide.h>
#include <deribit/sbe/multicast/Direction.h>
#include <deribit/sbe/multicast/InstrumentState.h>
#include <deribit/sbe/multicast/Liquidation.h>
#include <deribit/sbe/multicast/YesNo.h>

#include "roq/liquidity.hpp"
#include "roq/side.hpp"
#include "roq/trading_status.hpp"

#include "roq/map.hpp"

namespace roq {

template <>
template <>
std::optional<Side> Map<::deribit::sbe::multicast::BookSide::Value>::helper() const;

template <>
template <>
std::optional<Side> Map<::deribit::sbe::multicast::Direction::Value>::helper() const;

template <>
template <>
std::optional<TradingStatus> Map<::deribit::sbe::multicast::InstrumentState::Value>::helper() const;

template <>
template <>
std::optional<Liquidity> Map<::deribit::sbe::multicast::Liquidation::Value>::helper() const;

template <>
template <>
std::optional<bool> Map<::deribit::sbe::multicast::YesNo::Value>::helper() const;

}  // namespace roq
