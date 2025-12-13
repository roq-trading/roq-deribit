/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/sbe/map.hpp"

using namespace std::literals;

namespace roq {

namespace {
template <typename... Args>
using Helper = detail::MapHelper<Args...>;
}

// deribit => roq

// deribit_multicast::BookSide::Value => roq::Side

template <>
template <>
constexpr Helper<deribit_multicast::BookSide::Value>::operator std::optional<Side>() const {
  switch (std::get<0>(args_)) {
    using enum deribit_multicast::BookSide::Value;
    case ask:
      return Side::SELL;
    case bid:
      return Side::BUY;
    case NULL_VALUE:
      return Side::UNDEFINED;
  }
  return {};
}

static_assert(Helper{deribit_multicast::BookSide::Value{deribit_multicast::BookSide::ask}} == roq::Side::SELL);
static_assert(Helper{deribit_multicast::BookSide::Value{deribit_multicast::BookSide::bid}} == roq::Side::BUY);
static_assert(Helper{deribit_multicast::BookSide::Value{deribit_multicast::BookSide::NULL_VALUE}} == roq::Side::UNDEFINED);

template <>
template <>
std::optional<Side> Map<deribit_multicast::BookSide::Value>::helper() const {
  return Helper{args_};
}

// deribit_multicast::Direction::Value => roq::Side

template <>
template <>
constexpr Helper<deribit_multicast::Direction::Value>::operator std::optional<Side>() const {
  switch (std::get<0>(args_)) {
    using enum deribit_multicast::Direction::Value;
    case buy:
      return Side::BUY;
    case sell:
      return Side::SELL;
    case NULL_VALUE:
      return Side::UNDEFINED;
  }
  return {};
}

static_assert(Helper{deribit_multicast::Direction::Value{deribit_multicast::Direction::buy}} == roq::Side::BUY);
static_assert(Helper{deribit_multicast::Direction::Value{deribit_multicast::Direction::sell}} == roq::Side::SELL);
static_assert(Helper{deribit_multicast::Direction::Value{deribit_multicast::Direction::NULL_VALUE}} == roq::Side::UNDEFINED);

template <>
template <>
std::optional<Side> Map<deribit_multicast::Direction::Value>::helper() const {
  return Helper{args_};
}

// deribit_multicast::InstrumentState::Value => roq::TradingStatus

template <>
template <>
constexpr Helper<deribit_multicast::InstrumentState::Value>::operator std::optional<TradingStatus>() const {
  switch (std::get<0>(args_)) {
    using enum deribit_multicast::InstrumentState::Value;
    case created:
      return TradingStatus::OPEN;  // ???
    case open:
      return TradingStatus::OPEN;
    case closed:
      return TradingStatus::CLOSE;
    case settled:
      return TradingStatus::UNDEFINED;
    case inactive:
      return TradingStatus::UNDEFINED;
    case started:
      return TradingStatus::UNDEFINED;
    case deactivated:
      return TradingStatus::UNDEFINED;
    case NULL_VALUE:
      return TradingStatus::UNDEFINED;
  }
  return {};
}

static_assert(Helper{deribit_multicast::InstrumentState::Value{deribit_multicast::InstrumentState::created}} == roq::TradingStatus::OPEN);
static_assert(Helper{deribit_multicast::InstrumentState::Value{deribit_multicast::InstrumentState::open}} == roq::TradingStatus::OPEN);
static_assert(Helper{deribit_multicast::InstrumentState::Value{deribit_multicast::InstrumentState::closed}} == roq::TradingStatus::CLOSE);
static_assert(Helper{deribit_multicast::InstrumentState::Value{deribit_multicast::InstrumentState::settled}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit_multicast::InstrumentState::Value{deribit_multicast::InstrumentState::inactive}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit_multicast::InstrumentState::Value{deribit_multicast::InstrumentState::started}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit_multicast::InstrumentState::Value{deribit_multicast::InstrumentState::deactivated}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit_multicast::InstrumentState::Value{deribit_multicast::InstrumentState::NULL_VALUE}} == roq::TradingStatus::UNDEFINED);

template <>
template <>
std::optional<TradingStatus> Map<deribit_multicast::InstrumentState::Value>::helper() const {
  return Helper{args_};
}

// deribit_multicast::Liquidation::Value => roq::Liquidity

template <>
template <>
constexpr Helper<deribit_multicast::Liquidation::Value>::operator std::optional<Liquidity>() const {
  switch (std::get<0>(args_)) {
    using enum deribit_multicast::Liquidation::Value;
    case none:
      return Liquidity::UNDEFINED;
    case maker:
      return Liquidity::MAKER;
    case taker:
      return Liquidity::TAKER;
    case both:
      return Liquidity::UNDEFINED;  // ???
    case NULL_VALUE:
      return Liquidity::UNDEFINED;
  }
  return {};
}

static_assert(Helper{deribit_multicast::Liquidation::Value{deribit_multicast::Liquidation::none}} == roq::Liquidity::UNDEFINED);
static_assert(Helper{deribit_multicast::Liquidation::Value{deribit_multicast::Liquidation::maker}} == roq::Liquidity::MAKER);
static_assert(Helper{deribit_multicast::Liquidation::Value{deribit_multicast::Liquidation::taker}} == roq::Liquidity::TAKER);
static_assert(Helper{deribit_multicast::Liquidation::Value{deribit_multicast::Liquidation::both}} == roq::Liquidity::UNDEFINED);
static_assert(Helper{deribit_multicast::Liquidation::Value{deribit_multicast::Liquidation::NULL_VALUE}} == roq::Liquidity::UNDEFINED);

template <>
template <>
std::optional<Liquidity> Map<deribit_multicast::Liquidation::Value>::helper() const {
  return Helper{args_};
}

// deribit_multicast::YesNo::Value => std::bool

template <>
template <>
constexpr Helper<deribit_multicast::YesNo::Value>::operator std::optional<bool>() const {
  switch (std::get<0>(args_)) {
    using enum deribit_multicast::YesNo::Value;
    case no:
      return false;
    case yes:
      return true;
    case NULL_VALUE:
      return false;
  }
  return {};
}

static_assert(Helper{deribit_multicast::YesNo::Value{deribit_multicast::YesNo::no}} == false);
static_assert(Helper{deribit_multicast::YesNo::Value{deribit_multicast::YesNo::yes}} == true);
static_assert(Helper{deribit_multicast::YesNo::Value{deribit_multicast::YesNo::NULL_VALUE}} == false);

template <>
template <>
std::optional<bool> Map<deribit_multicast::YesNo::Value>::helper() const {
  return Helper{args_};
}

}  // namespace roq
