/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {

namespace {
template <typename... Args>
using Helper = detail::MapHelper<Args...>;
}

// deribit => roq

// deribit::protocol::json::Direction ==> roq::Side

template <>
template <>
constexpr Helper<deribit::protocol::json::Direction>::operator std::optional<roq::Side>() const {
  switch (std::get<0>(args_)) {
    using enum deribit::protocol::json::Direction::type_t;
    case UNDEFINED_INTERNAL:
      return roq::Side::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::Side::UNDEFINED;
    case BUY:
      return Side::BUY;
    case SELL:
      return Side::SELL;
    case ZERO:
      return roq::Side::UNDEFINED;
  }
  return {};
}

static_assert(Helper{deribit::protocol::json::Direction{deribit::protocol::json::Direction::UNDEFINED_INTERNAL}} == roq::Side::UNDEFINED);
static_assert(Helper{deribit::protocol::json::Direction{deribit::protocol::json::Direction::BUY}} == roq::Side::BUY);
static_assert(Helper{deribit::protocol::json::Direction{deribit::protocol::json::Direction::SELL}} == roq::Side::SELL);
static_assert(Helper{deribit::protocol::json::Direction{deribit::protocol::json::Direction::ZERO}} == roq::Side::UNDEFINED);

template <>
template <>
std::optional<roq::Side> Map<deribit::protocol::json::Direction>::helper() const {
  return Helper{args_};
}

// deribit::protocol::json::Liquidity ==> roq::Liquidity

template <>
template <>
constexpr Helper<deribit::protocol::json::Liquidity>::operator std::optional<roq::Liquidity>() const {
  switch (std::get<0>(args_)) {
    using enum deribit::protocol::json::Liquidity::type_t;
    case UNDEFINED_INTERNAL:
      return roq::Liquidity::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::Liquidity::UNDEFINED;
    case MAKER:
      return roq::Liquidity::MAKER;
    case TAKER:
      return roq::Liquidity::TAKER;
  }
  return {};
}

static_assert(Helper{deribit::protocol::json::Liquidity{deribit::protocol::json::Liquidity::UNDEFINED_INTERNAL}} == roq::Liquidity::UNDEFINED);
static_assert(Helper{deribit::protocol::json::Liquidity{deribit::protocol::json::Liquidity::MAKER}} == roq::Liquidity::MAKER);
static_assert(Helper{deribit::protocol::json::Liquidity{deribit::protocol::json::Liquidity::TAKER}} == roq::Liquidity::TAKER);

template <>
template <>
std::optional<roq::Liquidity> Map<deribit::protocol::json::Liquidity>::helper() const {
  return Helper{args_};
}

// deribit::protocol::json::State ==> roq::TradingStatus

template <>
template <>
constexpr Helper<deribit::protocol::json::State>::operator std::optional<roq::TradingStatus>() const {
  switch (std::get<0>(args_)) {
    using enum deribit::protocol::json::State::type_t;
    case UNDEFINED_INTERNAL:
      return TradingStatus::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return TradingStatus::UNDEFINED;
    case CLOSED:
      return TradingStatus::CLOSE;
    case OPEN:
      return TradingStatus::OPEN;
    case CREATED:
      return TradingStatus::UNDEFINED;
    case SETTLED:
      return TradingStatus::UNDEFINED;
    case TERMINATED:
      return TradingStatus::UNDEFINED;
    case INACTIVE:
      return TradingStatus::UNDEFINED;
    case DEACTIVATED:
      return TradingStatus::UNDEFINED;
    case STARTED:
      return TradingStatus::UNDEFINED;
    case SETTLEMENT:
      return TradingStatus::UNDEFINED;
    case DELIVERED:
      return TradingStatus::UNDEFINED;
    case LOCKED:
      return TradingStatus::UNDEFINED;
    case HALTED:
      return TradingStatus::CLOSE;
  }
  return {};
}

static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::UNDEFINED_INTERNAL}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::CLOSED}} == roq::TradingStatus::CLOSE);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::OPEN}} == roq::TradingStatus::OPEN);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::CREATED}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::SETTLED}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::TERMINATED}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::INACTIVE}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::DEACTIVATED}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::STARTED}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::SETTLEMENT}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::DELIVERED}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::LOCKED}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{deribit::protocol::json::State{deribit::protocol::json::State::HALTED}} == roq::TradingStatus::CLOSE);

template <>
template <>
std::optional<roq::TradingStatus> Map<deribit::protocol::json::State>::helper() const {
  return Helper{args_};
}

}  // namespace roq
