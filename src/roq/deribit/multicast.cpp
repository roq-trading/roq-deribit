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
  publish_stream_status();  // first message will publish
  if (!sbe::Parser::dispatch(*this, read.buffer)) {
    log::warn<5>("Failed to parse message"sv);
    log::warn<5>("{}"sv, debug::hex::Message{read.buffer});
  }
}

void Multicast::operator()(const core::net::UdpConnection::Error &error) {
  log::warn<1>("Error: what={}"sv, error.what);
}

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Instrument &instrument) {
  log::info<5>(
      "channel_id={}, sequence_number={}, instrument={}"sv,
      channel_id,
      sequence_number,
      instrument);
  auto instrument_id = instrument.instrumentId();
  if (!shared_.find_instrument_name(instrument_id, [](auto &) {})) {
    auto symbol = sbe::get_instrument_name(instrument);  // note! alloc
    assert(!std::empty(symbol));
    if (shared_.discard_symbol(symbol))
      return;
    shared_.instrument_names.try_emplace(instrument_id, symbol);
  }
}

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Book &book) {
  log::info<5>("channel_id={}, sequence_number={}, book={}"sv, channel_id, sequence_number, book);
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

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Quote &quote) {
  log::info<5>("channel_id={}, sequence_number={}, quote={}"sv, channel_id, sequence_number, quote);
  auto instrument_id = quote.instrumentId();
  shared_.find_instrument_name(instrument_id, [&](auto &symbol) {
    auto exchange_time_utc = std::chrono::milliseconds{quote.timestampMs()};
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
  });
}

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Trades &trades) {
  log::info<5>(
      "channel_id={}, sequence_number={}, trades={}"sv, channel_id, sequence_number, trades);
  auto instrument_id = trades.instrumentId();
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

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Snapshot &snapshot) {
  log::info<5>(
      "channel_id={}, sequence_number={}, snapshot={}"sv, channel_id, sequence_number, snapshot);
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

void Multicast::publish_stream_status() {
  if (initialized_)
    return;
  initialized_ = true;
  auto trace_info = server::create_trace_info();
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
