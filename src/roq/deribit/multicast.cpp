/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/multicast.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "roq/core/back_emplacer.hpp"

#include "roq/debug/hex/message.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/deribit/flags/common.hpp"
#include "roq/deribit/flags/config.hpp"
#include "roq/deribit/flags/multicast.hpp"

#include "roq/deribit/sbe/utils.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

namespace {
const auto NAME = "mc"sv;
const Mask SUPPORTS{
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context, auto port) {
  core::net::UdpConnection connection(handler, context, port);
  std::string local_interface{flags::Multicast::local_interface()};
  struct in_addr local = {};
  local.s_addr = inet_addr(local_interface.c_str());
  for (auto &multicast_address : flags::Multicast::multicast_address()) {
    struct in_addr multicast = {};
    multicast.s_addr = inet_addr(multicast_address.c_str());
    connection.add_membership(core::NetworkAddress{multicast, 0}, core::NetworkAddress{local, 0});
  }
  return connection;
}

bool test_sequence(auto &cache, auto instrument_id, auto sequence_number) {
  auto result = false;
  const constexpr uint32_t midpoint = 1 << 31;
  auto iter = cache.find(instrument_id);
  if (iter != cache.end()) {
    auto previous = (*iter).second;
    if (previous < sequence_number) {
      result = true;
    } else if (sequence_number < midpoint && midpoint < previous) {
      result = true;  // wraparound
    } else {
      // out of sequence
    }
  } else {
    iter = cache.emplace(instrument_id, sequence_number).first;
    result = true;
  }
  if (result)
    (*iter).second = sequence_number;
  return result;
}

template <typename T>
void emplace(MBPUpdate &result, const T &value) {
  new (&result) MBPUpdate{
      .price = value.price(),
      .quantity = value.amount(),
      .implied_quantity = NaN,
      .price_level = {},
      .number_of_orders = {},
  };
}

template <typename T>
void emplace(Trade &result, const T &value) {
  new (&result) Trade{
      .side = sbe::map_direction(value.direction()),
      .price = value.price(),
      .quantity = value.amount(),
      .trade_id = {},  // XXX value.tradeId() is uint64
  };
}
}  // namespace

Multicast::Multicast(
    Handler &handler, core::io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      publish_top_of_book_(!flags::Multicast::multicast_disable_top_of_book()),
      publish_market_by_price_(!flags::Multicast::multicast_disable_market_by_price()),
      publish_trade_summary_(!flags::Multicast::multicast_disable_trade_summary()),
      events_(create_connection(*this, context, flags::Multicast::multicast_port_events())),
      snapshot_(create_connection(*this, context, flags::Multicast::multicast_port_snapshot())),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
      },
      shared_(shared) {
}

void Multicast::operator()(const Event<Start> &) {
}

void Multicast::operator()(const Event<Stop> &) {
}

void Multicast::operator()(const Event<Timer> &) {
}

void Multicast::operator()(const core::net::UdpConnection::Read &read) {
  log::info<5>("received {} byte(s)"sv, std::size(read.buffer));
  auto trace_info = server::create_trace_info();
  publish_stream_status(trace_info);  // first message will publish
  if (!sbe::Parser::dispatch(*this, read.buffer, trace_info)) {
    log::warn<5>("Failed to parse message"sv);
    log::warn<5>("{}"sv, debug::hex::Message{read.buffer});
  }
}

void Multicast::operator()(const core::net::UdpConnection::Error &error) {
  log::warn<1>("Error: what={}"sv, error.what);
}

void Multicast::operator()(
    const Trace2<deribit_multicast::Instrument> &event, const sbe::Frame &frame) {
  auto &[trace_info, instrument] = event;
  log::info<5>("instrument={}, frame={}"sv, instrument, frame);
  auto instrument_id = instrument.instrumentId();
  if (!shared_.find_instrument_name(instrument_id, [](auto &) {})) {
    auto symbol = sbe::get_instrument_name(instrument);  // note! alloc
    assert(!std::empty(symbol));
    if (shared_.discard_symbol(symbol))
      return;
    shared_.instrument_names.try_emplace(instrument_id, symbol);
  }
}

void Multicast::operator()(const Trace2<deribit_multicast::Book> &event, const sbe::Frame &frame) {
  auto &[trace_info, book] = event;
  log::info<5>("book={}, frame={}"sv, book, frame);
  if (!publish_market_by_price_)
    return;
  auto instrument_id = book.instrumentId();
  shared_.find_instrument_name(instrument_id, [&](auto &symbol) {
    auto exchange_time_utc = std::chrono::milliseconds{book.timestampMs()};
    core::back_emplacer bids(shared_.bids), asks(shared_.asks);
    book.sbeRewind();
    book.changesList().forEach([&](auto &item) {
      auto side = sbe::map_book_side(item.side());
      switch (side) {
        using enum Side;
        case UNDEFINED:
          assert(false);
          break;
        case BUY:
          bids.emplace_back([&item](auto &result) { emplace(result, item); });
          break;
        case SELL:
          asks.emplace_back([&item](auto &result) { emplace(result, item); });
          break;
      }
    });
    const MarketByPriceUpdate market_by_price_update{
        .stream_id = stream_id_,
        .exchange = flags::Config::exchange(),
        .symbol = symbol,
        .bids = bids,
        .asks = asks,
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = static_cast<int64_t>(book.changeId()),
        .price_decimals = {},
        .quantity_decimals = {},
        .checksum = {},
    };
    log::info<3>("market_by_price_update={}"sv, market_by_price_update);
  });
}

void Multicast::operator()(const Trace2<deribit_multicast::Quote> &event, const sbe::Frame &frame) {
  auto &[trace_info, quote] = event;
  log::info<5>("quote={}, frame={}"sv, quote, frame);
  if (!publish_top_of_book_)
    return;
  auto instrument_id = quote.instrumentId();
  // note! skip previous updates
  if (test_sequence(last_quote_, instrument_id, frame.sequence_number)) {
    shared_.find_instrument_name(instrument_id, [&](auto &symbol) {
      auto exchange_time_utc = std::chrono::milliseconds{quote.timestampMs()};
      // note! unlike the WS feed, it looks like we do *not* have to scale amounts here
      const TopOfBook top_of_book{
          .stream_id = stream_id_,
          .exchange = flags::Config::exchange(),
          .symbol = symbol,
          .layer{
              .bid_price = quote.bestBidPrice(),
              .bid_quantity = quote.bestBidAmount(),
              .ask_price = quote.bestAskPrice(),
              .ask_quantity = quote.bestAskAmount(),
          },
          .update_type = UpdateType::INCREMENTAL,
          .exchange_time_utc = exchange_time_utc,
          .exchange_sequence = {},
      };
      log::info<3>("top_of_book={}"sv, top_of_book);
      create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
    });
  }
}

void Multicast::operator()(
    const Trace2<deribit_multicast::Trades> &event, const sbe::Frame &frame) {
  auto &[trace_info, trades] = event;
  log::info<5>("trades={}, frame={}"sv, trades, frame);
  if (!publish_trade_summary_)
    return;
  auto instrument_id = trades.instrumentId();
  // note! skip previous updates
  if (test_sequence(last_trades_, instrument_id, frame.sequence_number)) {
    shared_.find_instrument_name(instrument_id, [&](auto &symbol) {
      std::chrono::milliseconds exchange_time_utc{};
      core::back_emplacer trades_(shared_.trades);
      trades.sbeRewind();
      trades.tradesList().forEach([&](auto &item) {
        auto timestamp = std::chrono::milliseconds{item.timestampMs()};
        exchange_time_utc = std::max(exchange_time_utc, timestamp);
        trades_.emplace_back([&item](auto &result) { emplace(result, item); });
      });
      const TradeSummary trade_summary{
          .stream_id = stream_id_,
          .exchange = flags::Config::exchange(),
          .symbol = symbol,
          .trades = trades_,
          .exchange_time_utc = exchange_time_utc,
      };
      log::info<3>("trade_summary={}"sv, trade_summary);
    });
  }
}

void Multicast::operator()(
    const Trace2<deribit_multicast::Snapshot> &event, const sbe::Frame &frame) {
  auto &[trace_info, snapshot] = event;
  log::info<5>("snapshot={}, frame={}"sv, snapshot, frame);
  if (!publish_market_by_price_)
    return;
  auto instrument_id = snapshot.instrumentId();
  if (!shared_.find_instrument_name(instrument_id, [](auto &) {})) {
    auto symbol = sbe::get_instrument_name(snapshot);  // note! alloc
    assert(!std::empty(symbol));
    if (shared_.discard_symbol(symbol))
      return;
    shared_.instrument_names.try_emplace(instrument_id, symbol);
  }
}

void Multicast::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE);
}

void Multicast::publish_stream_status(const TraceInfo &trace_info) {
  if (initialized_)
    return;
  initialized_ = true;
  StreamStatus stream_status{
      .stream_id = stream_id_,
      .account = {},
      .supports = SUPPORTS,
      .transport = Transport::UDP,
      .protocol = Protocol::SBE,
      .encoding = {Encoding::SBE},
      .priority = Priority::PRIMARY,
      .connection_status = ConnectionStatus::READY,
  };
  create_trace_and_dispatch(handler_, trace_info, stream_status);
}

}  // namespace deribit
}  // namespace roq
