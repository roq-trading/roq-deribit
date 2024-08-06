/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/deribit/json/map.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace json {

// === HELPERS ===

namespace {
// note! constexpr helper for static testing
template <typename... Args>
struct Helper final {
  explicit constexpr Helper(std::tuple<Args...> const &args) : args_{args} {}
  explicit constexpr Helper(Args &&...args_) : args_{std::forward<Args>(args_)...} {}

  template <typename R>
  constexpr operator R();

 private:
  std::tuple<Args...> const args_;
};

// ==> roq

// Direction ==> roq::Side

template <>
template <>
constexpr Helper<Direction>::operator roq::Side() {
  switch (std::get<0>(args_)) {
    using enum Direction::type_t;
    case UNDEFINED__:
      return {};
    case UNKNOWN__:
      break;
    case BUY:
      return Side::BUY;
    case SELL:
      return Side::SELL;
    case ZERO:
      return {};
  }
  roq::log::fatal("Unexpected"sv);
}

static_assert(static_cast<roq::Side>(Helper{Direction{Direction::UNDEFINED__}}) == roq::Side::UNDEFINED);
static_assert(static_cast<roq::Side>(Helper{Direction{Direction::BUY}}) == roq::Side::BUY);
static_assert(static_cast<roq::Side>(Helper{Direction{Direction::SELL}}) == roq::Side::SELL);
static_assert(static_cast<roq::Side>(Helper{Direction{Direction::ZERO}}) == roq::Side::UNDEFINED);

// Liquidity ==> roq::Liquidity

template <>
template <>
constexpr Helper<Liquidity>::operator roq::Liquidity() {
  switch (std::get<0>(args_)) {
    using enum Liquidity::type_t;
    case UNDEFINED__:
      return {};
    case UNKNOWN__:
      break;
    case MAKER:
      return roq::Liquidity::MAKER;
    case TAKER:
      return roq::Liquidity::TAKER;
  }
  roq::log::fatal("Unexpected"sv);
}

static_assert(static_cast<roq::Liquidity>(Helper{Liquidity{Liquidity::UNDEFINED__}}) == roq::Liquidity::UNDEFINED);
static_assert(static_cast<roq::Liquidity>(Helper{Liquidity{Liquidity::MAKER}}) == roq::Liquidity::MAKER);
static_assert(static_cast<roq::Liquidity>(Helper{Liquidity{Liquidity::TAKER}}) == roq::Liquidity::TAKER);

// State ==> roq::TradingStatus

template <>
template <>
constexpr Helper<State>::operator roq::TradingStatus() {
  switch (std::get<0>(args_)) {
    using enum State::type_t;
    case UNDEFINED__:
      return {};
    case UNKNOWN__:
      break;
    case CLOSED:
      return TradingStatus::CLOSE;
    case OPEN:
      return TradingStatus::OPEN;
    case CREATED:
      return {};  // note!
    case SETTLED:
      return {};  // note!
    case TERMINATED:
      return {};  // note!
    case INACTIVE:
      return {};  // note!
    case DEACTIVATED:
      return {};  // note!
    case STARTED:
      return {};  // note!
  }
  roq::log::fatal("Unexpected"sv);
}

static_assert(static_cast<roq::TradingStatus>(Helper{State{State::UNDEFINED__}}) == roq::TradingStatus::UNDEFINED);
static_assert(static_cast<roq::TradingStatus>(Helper{State{State::CLOSED}}) == roq::TradingStatus::CLOSE);
static_assert(static_cast<roq::TradingStatus>(Helper{State{State::OPEN}}) == roq::TradingStatus::OPEN);
static_assert(static_cast<roq::TradingStatus>(Helper{State{State::CREATED}}) == roq::TradingStatus::UNDEFINED);
static_assert(static_cast<roq::TradingStatus>(Helper{State{State::SETTLED}}) == roq::TradingStatus::UNDEFINED);
static_assert(static_cast<roq::TradingStatus>(Helper{State{State::TERMINATED}}) == roq::TradingStatus::UNDEFINED);
static_assert(static_cast<roq::TradingStatus>(Helper{State{State::INACTIVE}}) == roq::TradingStatus::UNDEFINED);
static_assert(static_cast<roq::TradingStatus>(Helper{State{State::DEACTIVATED}}) == roq::TradingStatus::UNDEFINED);
static_assert(static_cast<roq::TradingStatus>(Helper{State{State::STARTED}}) == roq::TradingStatus::UNDEFINED);

// roq ==>

}  // namespace

// === IMPLEMENTATION ===

// ==> roq

template <>
template <>
Map<Direction>::operator roq::Side() {
  return Helper{args_};
}

template <>
template <>
Map<Liquidity>::operator roq::Liquidity() {
  return Helper{args_};
}

template <>
template <>
Map<State>::operator roq::TradingStatus() {
  return Helper{args_};
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
