/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <magic_enum/magic_enum_format.hpp>

#include <deribit/sbe/multicast/BookChange.h>
#include <deribit/sbe/multicast/BookSide.h>
#include <deribit/sbe/multicast/Direction.h>
#include <deribit/sbe/multicast/InstrumentState.h>
#include <deribit/sbe/multicast/Liquidation.h>
#include <deribit/sbe/multicast/YesNo.h>

#include <deribit/sbe/multicast/Book.h>           // 1001
#include <deribit/sbe/multicast/ComboLegs.h>      // 1007
#include <deribit/sbe/multicast/Instrument.h>     // 1000
#include <deribit/sbe/multicast/InstrumentV2.h>   // 1010
#include <deribit/sbe/multicast/PriceIndex.h>     // 1008
#include <deribit/sbe/multicast/Rfq.h>            // 1009
#include <deribit/sbe/multicast/Snapshot.h>       // 1004
#include <deribit/sbe/multicast/SnapshotEnd.h>    // 1006
#include <deribit/sbe/multicast/SnapshotStart.h>  // 1005
#include <deribit/sbe/multicast/Ticker.h>         // 1003
#include <deribit/sbe/multicast/Trades.h>         // 1002

#include "roq/api.hpp"

#include "roq/core/sbe/iterator.hpp"

#include "roq/deribit/sbe/map.hpp"

namespace roq {
namespace deribit {
namespace sbe {

template <typename T>
size_t compute_length(T &);

template <>
inline size_t compute_length(::deribit::sbe::multicast::Book &value) {
  auto changes_list_length = value.changesList().count();
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength(changes_list_length);
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::ComboLegs &value) {
  auto legs_list_length = value.legsList().count();
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength(legs_list_length);
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::Instrument &value) {
  auto instrument_name_length = value.instrumentNameLength();
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength(instrument_name_length);
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::InstrumentV2 &value) {
  value.sbeRewind();
  size_t tick_steps_list_length = 0;
  value.tickStepsList().forEach([&]([[maybe_unused]] auto &item) { ++tick_steps_list_length; });  // note!
  auto instrument_name_length = value.instrumentNameLength();
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength(tick_steps_list_length, instrument_name_length);
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::PriceIndex &value) {
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength();
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::Rfq &value) {
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength();
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::Snapshot &value) {
  auto levels_list_length = value.levelsList().count();
  value.sbeRewind();  // note!
  value.levelsList().forEach([](auto &item) { item.skip(); });
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength(levels_list_length);
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::SnapshotStart &value) {
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength();
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::SnapshotEnd &value) {
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength();
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::Ticker &value) {
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength();
}

template <>
inline size_t compute_length(::deribit::sbe::multicast::Trades &value) {
  auto trades_list_length = value.tradesList().count();
  using value_type = std::remove_cvref_t<decltype(value)>;
  return value_type::computeLength(trades_list_length);
}

// sbe weirdness ...

template <typename T>
std::string get_instrument_name(T &);

template <>
inline std::string get_instrument_name(::deribit::sbe::multicast::Instrument &value) {
  value.sbeRewind();                           // note!
  auto length = value.instrumentNameLength();  // must fetch before getting name
  return {value.instrumentName(), length};
}

template <>
inline std::string get_instrument_name(::deribit::sbe::multicast::InstrumentV2 &value) {
  value.sbeRewind();                                                   // note!
  value.tickStepsList().forEach([&]([[maybe_unused]] auto &item) {});  // note!
  auto length = value.instrumentNameLength();                          // must fetch before getting name
  return {value.instrumentName(), length};
}

}  // namespace sbe
}  // namespace deribit

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::Book>() {
  using namespace std::literals;
  return "book"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::ComboLegs>() {
  using namespace std::literals;
  return "combo_legs"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::Instrument>() {
  using namespace std::literals;
  return "instrument"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::InstrumentV2>() {
  using namespace std::literals;
  return "instrument_v2"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::PriceIndex>() {
  using namespace std::literals;
  return "price_index"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::Rfq>() {
  using namespace std::literals;
  return "rfq"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::Snapshot>() {
  using namespace std::literals;
  return "snapshot"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::SnapshotEnd>() {
  using namespace std::literals;
  return "snapshot_end"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::SnapshotStart>() {
  using namespace std::literals;
  return "snapshot_start"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::Ticker>() {
  using namespace std::literals;
  return "ticker"sv;
}

template <>
constexpr std::string_view get_name<::deribit::sbe::multicast::Trades>() {
  using namespace std::literals;
  return "trades"sv;
}
}  // namespace roq

// helper

template <>
struct fmt::formatter<::deribit::sbe::multicast::Book::ChangesList> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::Book::ChangesList const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(side={}, )"
        R"(change={}, )"
        R"(price={}, )"
        R"(amount={})"
        R"(}})"sv,
        ::deribit::sbe::multicast::BookSide::c_str(value.side()),
        ::deribit::sbe::multicast::BookChange::c_str(value.change()),
        value.price(),
        value.amount());
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::ComboLegs::LegsList> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::ComboLegs::LegsList const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instrumentId={}, )"
        R"(legSize={})"
        R"(}})"sv,
        value.legInstrumentId(),
        value.legSize());
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::InstrumentV2::TickStepsList> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::InstrumentV2::TickStepsList const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(abovePrice={}, )"
        R"(tickSize={})"
        R"(}})"sv,
        value.abovePrice(),
        value.tickSize());
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::Snapshot::LevelsList> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::Snapshot::LevelsList const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(side={}, )"
        R"(price={}, )"
        R"(amount={})"
        R"(}})"sv,
        ::deribit::sbe::multicast::BookSide::c_str(value.side()),
        value.price(),
        value.amount());
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::Trades::TradesList> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::Trades::TradesList const &value, format_context &context) const {
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
        ::deribit::sbe::multicast::Direction::c_str(value.direction()),
        value.price(),
        value.amount(),
        std::chrono::milliseconds{value.timestampMs()},
        value.markPrice(),
        value.indexPrice(),
        value.tradeSeq(),
        value.tradeId(),
        ::deribit::sbe::multicast::TickDirection::c_str(value.tickDirection()),
        ::deribit::sbe::multicast::Liquidation::c_str(value.liquidation()),
        value.iv(),
        value.blockTradeId(),
        value.comboTradeId());
  }
};

// messages
//
// note! some nested objects (lists) imply non-const due to positional information

template <>
struct fmt::formatter<::deribit::sbe::multicast::Book> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::Book &value, format_context &context) const {
    using namespace std::literals;
    value.sbeRewind();  // note!
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instrumentId={}, )"
        R"(timestampMs={}, )"
        R"(prevChangeId={}, )"
        R"(changeId={}, )"
        R"(isLast={}, )"
        R"(changesList=[{}])"
        R"(}})"sv,
        value.instrumentId(),
        std::chrono::milliseconds{value.timestampMs()},
        value.prevChangeId(),
        value.changeId(),
        static_cast<bool>(roq::map(value.isLast())),
        fmt::join(roq::core::sbe::iterator{value.changesList()}, roq::core::sbe::sentinel{}, ", "sv));
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::ComboLegs> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::ComboLegs &value, format_context &context) const {
    using namespace std::literals;
    value.sbeRewind();  // note!
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instrumentId={}, )"
        R"(legsList=[{}])"
        R"(}})"sv,
        value.instrumentId(),
        fmt::join(roq::core::sbe::iterator{value.legsList()}, roq::core::sbe::sentinel{}, ", "sv));
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::Instrument> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::Instrument &value, format_context &context) const {
    using namespace std::literals;
    auto instrument_name = roq::deribit::sbe::get_instrument_name(value);
    value.sbeRewind();  // note!
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instrumentId={}, )"
        R"(instrumentState={}, )"
        R"(kind={}, )"
        R"(instrumentType={}, )"
        R"(optionType={}, )"
        R"(rfq={}, )"
        R"(settlementPeriod={}, )"
        R"(settlementPeriodCount={}, )"
        R"(baseCurrency="{}", )"
        R"(quoteCurrency="{}", )"
        R"(counterCurrency="{}", )"
        R"(settlementCurrency="{}", )"
        R"(sizeCurrency="{}", )"
        R"(creationTimestampMs={}, )"
        R"(expirationTimestampMs={}, )"
        R"(strikePrice={}, )"
        R"(contractSize={}, )"
        R"(minTradeAmount={}, )"
        R"(tickSize={}, )"
        R"(makerCommission={}, )"
        R"(takerCommission={}, )"
        R"(blockTradeCommission={}, )"
        R"(maxLiquidationCommission={}, )"
        R"(maxLeverage={}, )"
        R"(instrumentName="{}")"
        R"(}})"sv,
        value.instrumentId(),
        ::deribit::sbe::multicast::InstrumentState::c_str(value.instrumentState()),
        ::deribit::sbe::multicast::InstrumentKind::c_str(value.kind()),
        ::deribit::sbe::multicast::InstrumentType::c_str(value.instrumentType()),
        ::deribit::sbe::multicast::OptionType::c_str(value.optionType()),
        static_cast<bool>(roq::map(value.rfq())),
        ::deribit::sbe::multicast::Period::c_str(value.settlementPeriod()),
        value.settlementPeriodCount(),
        value.baseCurrency(),
        value.quoteCurrency(),
        value.counterCurrency(),
        value.settlementCurrency(),
        value.sizeCurrency(),
        std::chrono::milliseconds{value.creationTimestampMs()},
        std::chrono::milliseconds{value.expirationTimestampMs()},
        value.strikePrice(),
        value.contractSize(),
        value.minTradeAmount(),
        value.tickSize(),
        value.makerCommission(),
        value.takerCommission(),
        value.blockTradeCommission(),
        value.maxLiquidationCommission(),
        value.maxLeverage(),
        instrument_name);
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::InstrumentV2> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::InstrumentV2 &value, format_context &context) const {
    using namespace std::literals;
    auto instrument_name = roq::deribit::sbe::get_instrument_name(value);
    value.sbeRewind();  // note!
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instrumentId={}, )"
        R"(instrumentState={}, )"
        R"(kind={}, )"
        R"(instrumentType={}, )"
        R"(optionType={}, )"
        R"(settlementPeriod={}, )"
        R"(settlementPeriodCount={}, )"
        R"(baseCurrency="{}", )"
        R"(quoteCurrency="{}", )"
        R"(counterCurrency="{}", )"
        R"(settlementCurrency="{}", )"
        R"(sizeCurrency="{}", )"
        R"(creationTimestampMs={}, )"
        R"(expirationTimestampMs={}, )"
        R"(strikePrice={}, )"
        R"(contractSize={}, )"
        R"(minTradeAmount={}, )"
        R"(tickSize={}, )"
        R"(makerCommission={}, )"
        R"(takerCommission={}, )"
        R"(blockTradeCommission={}, )"
        R"(maxLiquidationCommission={}, )"
        R"(maxLeverage={}, )"
        R"(tickStepsList=[{}], )"
        R"(instrumentName="{}")"
        R"(}})"sv,
        value.instrumentId(),
        ::deribit::sbe::multicast::InstrumentState::c_str(value.instrumentState()),
        ::deribit::sbe::multicast::InstrumentKind::c_str(value.kind()),
        ::deribit::sbe::multicast::InstrumentType::c_str(value.instrumentType()),
        ::deribit::sbe::multicast::OptionType::c_str(value.optionType()),
        ::deribit::sbe::multicast::Period::c_str(value.settlementPeriod()),
        value.settlementPeriodCount(),
        value.baseCurrency(),
        value.quoteCurrency(),
        value.counterCurrency(),
        value.settlementCurrency(),
        value.sizeCurrency(),
        std::chrono::milliseconds{value.creationTimestampMs()},
        std::chrono::milliseconds{value.expirationTimestampMs()},
        value.strikePrice(),
        value.contractSize(),
        value.minTradeAmount(),
        value.tickSize(),
        value.makerCommission(),
        value.takerCommission(),
        value.blockTradeCommission(),
        value.maxLiquidationCommission(),
        value.maxLeverage(),
        fmt::join(roq::core::sbe::iterator{value.tickStepsList()}, roq::core::sbe::sentinel{}, ", "sv),
        instrument_name);
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::Ticker> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::Ticker &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instrumentId={}, )"
        R"(instrumentState={}, )"
        R"(timestampMs={}, )"
        R"(openInterest={}, )"
        R"(minSellPrice={}, )"
        R"(maxBuyPrice={}, )"
        R"(lastPrice={}, )"
        R"(indexPrice={}, )"
        R"(markPrice={}, )"
        R"(bestBidPrice={}, )"
        R"(bestBidAmount={}, )"
        R"(bestAskPrice={}, )"
        R"(bestAskAmount={}, )"
        R"(currentFunding={}, )"
        R"(funding8h={}, )"
        R"(estimatedDeliveryPrice={}, )"
        R"(deliveryPrice={}, )"
        R"(settlementPrice={})"
        R"(}})"sv,
        value.instrumentId(),
        ::deribit::sbe::multicast::InstrumentState::c_str(value.instrumentState()),
        std::chrono::milliseconds{value.timestampMs()},
        value.openInterest(),
        value.minSellPrice(),
        value.maxBuyPrice(),
        value.lastPrice(),
        value.indexPrice(),
        value.markPrice(),
        value.bestBidPrice(),
        value.bestBidAmount(),
        value.bestAskPrice(),
        value.bestAskAmount(),
        value.currentFunding(),
        value.funding8h(),
        value.estimatedDeliveryPrice(),
        value.deliveryPrice(),
        value.settlementPrice());
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::PriceIndex> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::PriceIndex &value, format_context &context) const {
    using namespace std::literals;
    value.sbeRewind();  // note!
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(indexName="{}", )"
        R"(price={}, )"
        R"(timestampMs={})"
        R"(}})"sv,
        value.indexName(),
        value.price(),
        std::chrono::milliseconds{value.timestampMs()});
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::Rfq> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::Rfq &value, format_context &context) const {
    using namespace std::literals;
    value.sbeRewind();  // note!
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instrumentId={}, )"
        R"(state={}, )"
        R"(side={}, )"
        R"(amount={}, )"
        R"(timestampMs={})"
        R"(}})"sv,
        value.instrumentId(),
        static_cast<bool>(roq::map(value.state())),
        ::deribit::sbe::multicast::RfqDirection::c_str(value.side()),
        value.amount(),
        std::chrono::milliseconds{value.timestampMs()});
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::Snapshot> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::Snapshot &value, format_context &context) const {
    using namespace std::literals;
    value.sbeRewind();  // note!
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instrumentId={}, )"
        R"(timestampMs={}, )"
        R"(changeId={}, )"
        R"(isBookComplete={}, )"
        R"(isLastInBook={}, )"
        R"(levelsList=[{}])"
        R"(}})"sv,
        value.instrumentId(),
        std::chrono::milliseconds{value.timestampMs()},
        value.changeId(),
        static_cast<bool>(roq::map(value.isBookComplete())),
        static_cast<bool>(roq::map(value.isLastInBook())),
        fmt::join(roq::core::sbe::iterator{value.levelsList()}, roq::core::sbe::sentinel{}, ", "sv));
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::SnapshotStart> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::SnapshotStart &value, format_context &context) const {
    using namespace std::literals;
    value.sbeRewind();  // note!
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(snapshotDelay={})"
        R"(}})"sv,
        value.snapshotDelay());
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::SnapshotEnd> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::SnapshotEnd &, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{{}}"sv);
  }
};

template <>
struct fmt::formatter<::deribit::sbe::multicast::Trades> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(::deribit::sbe::multicast::Trades &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instrumentId={}, )"
        R"(tradesList=[{}])"
        R"(}})"sv,
        value.instrumentId(),
        fmt::join(roq::core::sbe::iterator{value.tradesList()}, roq::core::sbe::sentinel{}, ", "sv));
  }
};
