/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <deribit_multicast/BookChange.h>
#include <deribit_multicast/BookSide.h>
#include <deribit_multicast/Direction.h>
#include <deribit_multicast/InstrumentState.h>
#include <deribit_multicast/Liquidation.h>
#include <deribit_multicast/YesNo.h>

#include <deribit_multicast/Book.h>
#include <deribit_multicast/Instrument.h>
#include <deribit_multicast/Quote.h>
#include <deribit_multicast/Snapshot.h>
#include <deribit_multicast/Trades.h>

#include "roq/api.hpp"

#include "roq/core/sbe/iterator.hpp"

namespace roq {
namespace deribit {
namespace sbe {

inline Side map_book_side(deribit_multicast::BookSide::Value value) {
  switch (value) {
    using enum deribit_multicast::BookSide::Value;
    case ask:
      return Side::SELL;
    case bid:
      return Side::BUY;
    case NULL_VALUE:
      return Side::UNDEFINED;
  }
  return Side::UNDEFINED;
}

inline Side map_direction(deribit_multicast::Direction::Value value) {
  switch (value) {
    using enum deribit_multicast::Direction::Value;
    case buy:
      return Side::BUY;
    case sell:
      return Side::SELL;
    case NULL_VALUE:
      return Side::UNDEFINED;
  }
  return Side::UNDEFINED;
}

inline TradingStatus map_instrument_state(deribit_multicast::InstrumentState::Value value) {
  switch (value) {
    using enum deribit_multicast::InstrumentState::Value;
    case created:
      return TradingStatus::OPEN;  // ???
    case open:
      return TradingStatus::OPEN;
    case closed:
      return TradingStatus::CLOSE;
    case settled:
      break;  // ???
    case NULL_VALUE:
      return TradingStatus::UNDEFINED;
  }
  return TradingStatus::UNDEFINED;
}

inline Liquidity map_liquidation(deribit_multicast::Liquidation::Value value) {
  switch (value) {
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
  return Liquidity::UNDEFINED;
}

inline bool map_yes_no(deribit_multicast::YesNo::Value value) {
  switch (value) {
    using enum deribit_multicast::YesNo::Value;
    case no:
      return false;
    case yes:
      return true;
    case NULL_VALUE:
      return false;
  }
  return false;
}

}  // namespace sbe
}  // namespace deribit
}  // namespace roq

// header

template <>
struct fmt::formatter<deribit_multicast::MessageHeader> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(const deribit_multicast::MessageHeader &value, Context &context) {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(blockLength={}, )"
        R"(templateId={}, )"
        R"(schemaId={}, )"
        R"(version={}, )"
        R"(numGroups={}, )"
        R"(numVarDataFields={})"
        R"(}})"sv,
        value.blockLength(),
        value.templateId(),
        value.schemaId(),
        value.version(),
        value.numGroups(),
        value.numVarDataFields());
  }
};

// helper

template <>
struct fmt::formatter<deribit_multicast::Book::ChangesList> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(const deribit_multicast::Book::ChangesList &value, Context &context) {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(side={}, )"
        R"(change={}, )"
        R"(price={}, )"
        R"(amount={})"
        R"(}})"sv,
        deribit_multicast::BookSide::c_str(value.side()),
        deribit_multicast::BookChange::c_str(value.change()),
        value.price(),
        value.amount());
  }
};

template <>
struct fmt::formatter<deribit_multicast::Trades::TradesList> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(const deribit_multicast::Trades::TradesList &value, Context &context) {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(direction={}, )"
        R"(price={}, )"
        R"(amount={}, )"
        R"(timestampMs={}, )"
        R"(markPrice={}, )"
        R"(indexPrice={}, )"
        R"(tradeSeq={}, )"
        R"(tradeId={}, )"
        R"(tickDirection={}, )"
        R"(liquidation={}, )"
        R"(iv={}, )"
        R"(blockTradeId={}, )"
        R"(comboTradeId={})"
        R"(}})"sv,
        deribit_multicast::Direction::c_str(value.direction()),
        value.price(),
        value.amount(),
        std::chrono::milliseconds{value.timestampMs()},
        value.markPrice(),
        value.indexPrice(),
        value.tradeSeq(),
        value.tradeId(),
        deribit_multicast::TickDirection::c_str(value.tickDirection()),
        value.iv(),
        value.blockTradeId(),
        value.comboTradeId());
  }
};

template <>
struct fmt::formatter<deribit_multicast::Snapshot::LevelsList> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(const deribit_multicast::Snapshot::LevelsList &value, Context &context) {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(side={}, )"
        R"(price={}, )"
        R"(amount={})"
        R"(}})"sv,
        deribit_multicast::BookSide::c_str(value.side()),
        value.price(),
        value.amount());
  }
};

// messages
//
// note! some nested objects (lists) imply non-const due to positional information

template <>
struct fmt::formatter<deribit_multicast::Book> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(deribit_multicast::Book &value, Context &context) {
    using namespace std::literals;
    value.sbeRewind();
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(header={}, )"
        R"(instrumentId={}, )"
        R"(timestampMs={}, )"
        R"(prevChangeId={}, )"
        R"(changeId={}, )"
        R"(isLast={}, )"
        R"(changesList=[{}])"
        R"(}})"sv,
        value.header(),
        value.instrumentId(),
        std::chrono::milliseconds{value.timestampMs()},
        value.prevChangeId(),
        value.changeId(),
        roq::deribit::sbe::map_yes_no(value.isLast()),
        fmt::join(
            roq::core::sbe::iterator{value.changesList()}, roq::core::sbe::sentinel{}, ", "sv));
  }
};

template <>
struct fmt::formatter<deribit_multicast::Instrument> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(deribit_multicast::Instrument &value, Context &context) {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(header={}, )"
        R"(instrumentId={}, )"
        R"(state={}, )"
        R"(instrumentName="{}")"
        R"(}})"sv,
        value.header(),
        value.instrumentId(),
        deribit_multicast::InstrumentState::c_str(value.state()),
        value.instrumentName());
  }
};

template <>
struct fmt::formatter<deribit_multicast::Quote> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(deribit_multicast::Quote &value, Context &context) {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(header={}, )"
        R"(instrumentId={}, )"
        R"(timestampMs={}, )"
        R"(bestBidPrice={}, )"
        R"(bestBidAmount={}, )"
        R"(bestAskPrice={}, )"
        R"(bestAskAmount={})"
        R"(}})"sv,
        value.header(),
        value.instrumentId(),
        std::chrono::milliseconds{value.timestampMs()},
        value.bestBidPrice(),
        value.bestBidAmount(),
        value.bestAskPrice(),
        value.bestAskAmount());
  }
};

template <>
struct fmt::formatter<deribit_multicast::Snapshot> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(deribit_multicast::Snapshot &value, Context &context) {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(header={}, )"
        R"(instrumentId={}, )"
        R"(isBookComplete={}, )"
        R"(isLastInBook={}, )"
        R"(timestampMs={}, )"
        R"(prevChangeId={}, )"
        R"(changeId={}, )"
        R"(levelsList=[{}])"
        R"(}})"sv,
        value.header(),
        value.instrumentId(),
        roq::deribit::sbe::map_yes_no(value.isBookComplete()),
        roq::deribit::sbe::map_yes_no(value.isLastInBook()),
        std::chrono::milliseconds{value.timestampMs()},
        value.changeId(),
        fmt::join(
            roq::core::sbe::iterator{value.levelsList()}, roq::core::sbe::sentinel{}, ", "sv));
  }
};

template <>
struct fmt::formatter<deribit_multicast::Trades> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(deribit_multicast::Trades &value, Context &context) {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(header={}, )"
        R"(instrumentId={}, )"
        R"(tradesList=[{}])"
        R"(}})"sv,
        value.header(),
        value.instrumentId(),
        fmt::join(
            roq::core::sbe::iterator{value.tradesList()}, roq::core::sbe::sentinel{}, ", "sv));
  }
};

