/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/udp_snapshot.hpp"

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
auto const NAME = "udps"sv;
const Mask SUPPORTS{
    SupportType::MARKET_BY_PRICE,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(std::string_view const &group, std::string_view const &function)
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
}  // namespace

UDPSnapshot::UDPSnapshot(Handler &handler, core::io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      publish_market_by_price_(!flags::Multicast::multicast_disable_market_by_price()),
      connection_(create_connection(*this, context, flags::Multicast::multicast_port_snapshot())),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
      },
      shared_(shared), aggregator_(server::Flags::cache_mbp_max_depth()) {
  log::info<5>("DEBUG: publish_market_by_price={}"sv, publish_market_by_price_);
}

void UDPSnapshot::operator()(Event<Start> const &) {
}

void UDPSnapshot::operator()(Event<Stop> const &) {
}

void UDPSnapshot::operator()(Event<Timer> const &) {
}

void UDPSnapshot::operator()(core::net::UdpConnection::Read const &read) {
  log::info<5>("received {} byte(s)"sv, std::size(read.buffer));
  auto trace_info = server::create_trace_info();
  publish_stream_status(trace_info);  // first message will publish
  if (!sbe::Parser::dispatch(*this, read.buffer, trace_info)) {
    log::warn<5>("Failed to parse message"sv);
    log::warn<5>("{}"sv, debug::hex::Message{read.buffer});
  }
}

void UDPSnapshot::operator()(core::net::UdpConnection::Error const &error) {
  log::warn<1>("Error: what={}"sv, error.what);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::Instrument> const &event, sbe::Frame const &frame) {
  auto &instrument = event.value;
  log::info<5>("instrument={}, frame={}"sv, instrument, frame);
  if (aggregator_(frame.sequence_number)) {
    // note! always include
    auto const instrument_id = instrument.instrumentId();
    shared_.find_instrument_name_with_create(instrument_id, [&]() {
      return sbe::get_instrument_name(instrument);  // note! alloc
    });
  }
}

void UDPSnapshot::operator()(Trace<deribit_multicast::Book> const &event, sbe::Frame const &frame) {
  auto &book = event.value;
  log::info<5>("book={}, frame={}"sv, book, frame);
  log::fatal("Unexpected"sv);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::Ticker> const &event, sbe::Frame const &frame) {
  auto &ticker = event.value;
  log::info<5>("ticker={}, frame={}"sv, ticker, frame);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::Trades> const &event, sbe::Frame const &frame) {
  auto &trades = event.value;
  log::info<5>("trades={}, frame={}"sv, trades, frame);
  log::fatal("Unexpected"sv);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::Snapshot> const &event, sbe::Frame const &frame) {
  auto &trace_info = event.trace_info;
  auto &snapshot = event.value;
  log::info<5>("snapshot={}, frame={}"sv, snapshot, frame);
  auto const instrument_id = snapshot.instrumentId();
  auto const change_id = snapshot.changeId();
  auto const is_last = snapshot.isLastInBook();
  aggregator_(frame.sequence_number, instrument_id, change_id, is_last, [&](auto &bids, auto &asks) {
    if (!publish_market_by_price_)
      return;
    if (shared_.find_instrument_name(instrument_id, [&](auto &symbol) {
          snapshot.sbeRewind();
          snapshot.levelsList().forEach([&](auto const &item) { emplace_back(item, bids, asks); });
          log::info<5>(
              "DEBUG: change_id={}, bids=[{}], asks=[{}]"sv, change_id, fmt::join(bids, ","sv), fmt::join(asks, ","sv));
          // XXX allow empty? (after all, we need to record the sequence number...)
          if (is_last && !(std::empty(bids) && std::empty(asks))) {
            std::chrono::milliseconds const timestamp{snapshot.timestampMs()};
            auto &collector = shared_.mbp_collector[symbol];
            collector(
                bids,
                asks,
                change_id,
                [&](auto &bids, auto &asks, auto sequence) {  // snapshot
                  log::info<1>(R"(Received snapshot: symbol="{}")"sv, symbol);
                  // log::debug(R"(PUBLISH SNAPSHOT symbol="{}", sequence={})"sv, symbol, sequence);
                  log::info<5>(
                      R"(DEBUG: PUBLISH SNAPSHOT symbol="{}", sequence={}, change_id={}, timestamp={})"sv,
                      symbol,
                      sequence,
                      change_id,
                      timestamp);
                  const MarketByPriceUpdate market_by_price_update{
                      .stream_id = stream_id_,
                      .exchange = flags::Config::exchange(),
                      .symbol = symbol,
                      .bids = bids,
                      .asks = asks,
                      .update_type = UpdateType::SNAPSHOT,
                      .exchange_time_utc = timestamp,
                      .exchange_sequence = sequence,
                      .price_decimals = {},
                      .quantity_decimals = {},
                      .checksum = {},
                  };
                  log::info<5>("DEBUG: BEFORE market_by_price={}"sv, market_by_price_update);
                  Trace event(trace_info, market_by_price_update);
                  shared_(
                      event, true, [&](auto &market_by_price) { collector.apply(market_by_price, sequence, true); });
                },
                [&](auto retries) {  // request
                  log::info<1>(R"(Waiting for snapshot: symbol="{}")"sv, symbol);
                  // log::debug(R"(REQUEST symbol="{}" (retries={}))"sv, symbol, retries);
                  log::info<5>(R"(DEBUG: REQUEST symbol="{}" (retries={}))"sv, symbol, retries);
                  // note! don't have to do anything -- just wait for snapshot
                });
          }
        })) {
    } else {
      // unknown instrument_id
      log::info<5>("DEBUG: unknown instrument_id={}"sv, instrument_id);
    }
  });
}

void UDPSnapshot::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE);
}

void UDPSnapshot::publish_stream_status(TraceInfo const &trace_info) {
  if (initialized_)
    return;
  initialized_ = true;
  const StreamStatus stream_status{
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

template <typename T, typename U>
void UDPSnapshot::emplace_back(const T &item, U &bids, U &asks) {
  const MBPUpdate mbp_update{
      .price = item.price(),
      .quantity = item.amount(),
      .implied_quantity = NaN,
      .price_level = {},
      .number_of_orders = {},
  };
  auto side = sbe::map_book_side(deribit_multicast::BookSide::get(item.side()));
  switch (side) {
    case Side::UNDEFINED:
      assert(false);
      break;
    case Side::BUY:
      bids.emplace_back(std::move(mbp_update));
      break;
    case Side::SELL:
      asks.emplace_back(std::move(mbp_update));
      break;
  }
}

}  // namespace deribit
}  // namespace roq
