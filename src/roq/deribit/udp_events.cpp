/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/udp_events.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "roq/utils/update.hpp"

#include "roq/core/charconv/number.hpp"

#include "roq/debug/hex/message.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/io/network_address.hpp"

#include "roq/deribit/utils.hpp"

#include "roq/deribit/flags/common.hpp"
#include "roq/deribit/flags/config.hpp"
#include "roq/deribit/flags/multicast.hpp"

#include "roq/deribit/sbe/utils.hpp"

using namespace std::literals;

using namespace fmt::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const NAME = "udpe"sv;
}

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"_cf, stream_id, NAME);
}

auto publish_top_of_book() {
  return !flags::Multicast::multicast_disable_top_of_book();
}

auto publish_market_by_price() {
  return !flags::Multicast::multicast_disable_market_by_price();
}

auto publish_trade_summary() {
  return !flags::Multicast::multicast_disable_trade_summary();
}

auto get_supports(auto publish_top_of_book, auto publish_market_by_price, auto publish_trade_summary) {
  Mask<SupportType> result;
  if (publish_top_of_book)
    result |= SupportType::TOP_OF_BOOK;
  if (publish_market_by_price)
    result |= SupportType::MARKET_BY_PRICE;
  if (publish_trade_summary)
    result |= SupportType::TRADE_SUMMARY;
  return result;
}

auto create_receiver(auto &handler, auto &context, auto port) {
  log::info<1>("Create multicast socket port={}"sv, port);
  auto network_address = io::NetworkAddress{port};
  auto socket_options = Mask{
      io::SocketOption::REUSE_ADDRESS,
  };
  auto receiver = context.create_udp_receiver(handler, network_address, socket_options);
  log::info<1>(R"(Local interface is "{}")"sv, flags::Multicast::local_interface());
  std::string local_interface{flags::Multicast::local_interface()};
  struct in_addr local = {};
  local.s_addr = inet_addr(local_interface.c_str());
  for (auto &multicast_address : flags::Multicast::multicast_address()) {
    log::info<1>(R"(Add membership "{}")"sv, multicast_address);
    struct in_addr multicast = {};
    multicast.s_addr = inet_addr(multicast_address.c_str());
    (*receiver).add_membership(io::NetworkAddress{0, multicast}, io::NetworkAddress{0, local});
  }
  return receiver;
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(auto const &group, auto const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

// following is used from several places

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
}  // namespace

// === IMPLEMENTATION ===

UDPEvents::UDPEvents(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)},
      publish_top_of_book_{publish_top_of_book()}, publish_market_by_price_{publish_market_by_price()},
      publish_trade_summary_{publish_trade_summary()},
      supports_{get_supports(publish_top_of_book_, publish_market_by_price_, publish_trade_summary_)},
      receiver_{create_receiver(*this, context, flags::Multicast::multicast_port_events())},
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
      },
      shared_{shared} {
  log::info("DEBUG: publish_top_of_book={}"sv, publish_top_of_book_);
  log::info("DEBUG: publish_market_by_price={}"sv, publish_market_by_price_);
  log::info("DEBUG: publish_trade_summary={}"sv, publish_trade_summary_);
}

void UDPEvents::operator()(Event<Start> const &) {
  TraceInfo trace_info;
  publish_stream_status(trace_info, ConnectionStatus::CONNECTING);
  last_update_time_ = trace_info.source_receive_time;
}

void UDPEvents::operator()(Event<Stop> const &) {
}

void UDPEvents::operator()(Event<Timer> const &event) {
  if (last_update_time_.count() && (last_update_time_ + flags::Multicast::multicast_timeout()) < event.value.now) {
    log::warn("*** DETECTED TIMEOUT ***"sv);
    last_update_time_ = {};
  }
}

void UDPEvents::operator()(io::net::udp::Receiver::Read const &) {
  TraceInfo trace_info;
  last_update_time_ = trace_info.source_receive_time;
  publish_stream_status(trace_info, ConnectionStatus::READY);  // first message will publish
  while (receive_buffer_.append(*receiver_)) {
    auto message = receive_buffer_.data();
    log::info<5>("received {} byte(s)"sv, std::size(message));
    auto bytes = sbe::Parser::dispatch(*this, message, trace_info);
    if (!bytes || bytes != std::size(message)) {
      log::warn("{}"sv, debug::hex::Message{message});
      log::fatal("Failed to parse message"sv);
    }
    receive_buffer_.drain(bytes);  // XXX clear()?
  }
}

void UDPEvents::operator()(io::net::udp::Receiver::Error const &error) {
  log::fatal("Error: what={}"sv, error.what);
}

void UDPEvents::operator()(Trace<deribit_multicast::Instrument> const &event, sbe::Frame const &frame) {
  using value_type = std::remove_cvref<decltype(event)>::type::value_type;
  auto &instrument = const_cast<value_type &>(event.value);  // note! not const-safe
  log::info<2>("instrument={}, frame={}"sv, instrument, frame);
  auto &aggregator = get_aggregator(frame.channel_id);
  if (aggregator(frame.sequence_number)) {
    // note! always include
    auto const instrument_id = instrument.instrumentId();
    shared_.find_instrument_name_with_create(instrument_id, [&]() {
      auto contract_size = instrument.contractSize();
      auto multiplier = compute_contracts_multiplier(contract_size);
      return Instrument{
          sbe::get_instrument_name(instrument),  // note! alloc
          contract_size,
          multiplier,
      };
    });
  }
}

void UDPEvents::operator()(Trace<deribit_multicast::Book> const &event, sbe::Frame const &frame) {
  auto &trace_info = event.trace_info;
  using value_type = std::remove_cvref<decltype(event)>::type::value_type;
  auto &book = const_cast<value_type &>(event.value);  // note! not const-safe
  log::info<4>("book={}, frame={}"sv, book, frame);
  auto const instrument_id = book.instrumentId();
  auto const change_id = book.changeId();
  auto const is_last = book.isLast();
  auto &aggregator = get_aggregator(frame.channel_id);
  aggregator(frame.sequence_number, instrument_id, change_id, is_last, [&](auto &bids, auto &asks) {
    if (!publish_market_by_price_)
      return;
    if (shared_.find_instrument(instrument_id, [&](auto &instrument) {
          auto const prev_change_id = book.prevChangeId();
          std::chrono::milliseconds const timestamp{book.timestampMs()};
          book.sbeRewind();
          book.changesList().forEach([&](auto &item) { emplace_back(item, instrument.multiplier, bids, asks); });
          auto &collector = shared_.mbp_collector[instrument.symbol];
          try {
            auto create_update =
                [&](auto &bids, auto &asks, auto update_type, auto exchange_sequence) -> MarketByPriceUpdate {
              return {
                  .stream_id = stream_id_,
                  .exchange = flags::Config::exchange(),
                  .symbol = instrument.symbol,
                  .bids = bids,
                  .asks = asks,
                  .update_type = update_type,
                  .exchange_time_utc = timestamp,
                  .exchange_sequence = exchange_sequence,
                  .price_decimals = {},
                  .quantity_decimals = {},
                  .checksum = {},
              };
            };
            auto publish_update = [&](auto &bids, auto &asks) {
              // log::debug(R"(PUBLISH UPDATE symbol="{}")"sv, instrument.symbol);
              log::info<5>(
                  R"(DEBUG: PUBLISH UPDATE symbol="{}", change_id={}, prev_change_id={})"sv,
                  instrument.symbol,
                  change_id,
                  prev_change_id);
              auto market_by_price_update =
                  create_update(bids, asks, UpdateType::INCREMENTAL, static_cast<int64_t>(change_id));
              create_trace_and_dispatch(handler_, trace_info, market_by_price_update, true);
            };
            auto publish_snapshot = [&](auto &bids, auto &asks, auto sequence) {
              // log::debug(R"(PUBLISH SNAPSHOT symbol="{}", sequence={})"sv, instrument.symbol, sequence);
              log::info<5>(
                  R"(DEBUG: PUBLISH SNAPSHOT symbol="{}", sequence={}, change_id={}, prev_change_id={})"sv,
                  instrument.symbol,
                  sequence,
                  change_id,
                  prev_change_id);
              auto market_by_price_update = create_update(bids, asks, UpdateType::SNAPSHOT, sequence);
              auto apply_updates = [&](auto &market_by_price) { collector.apply(market_by_price, sequence, true); };
              Trace event{trace_info, market_by_price_update};
              shared_(event, true, apply_updates);
            };
            auto request_snapshot = [&](auto retries) {
              log::info<1>(R"(Waiting for snapshot: symbol="{}")"sv, instrument.symbol);
              // log::debug(R"(REQUEST symbol="{}" (retries={}))"sv, instrument.symbol, retries);
              log::info<5>(R"(DEBUG: REQUEST symbol="{}" (retries={}))"sv, instrument.symbol, retries);
              // note! don't have to do anything -- just wait for snapshot
            };
            collector(
                bids, asks, change_id, change_id, prev_change_id, publish_update, publish_snapshot, request_snapshot);
          } catch (BadState &) {
            log::fatal("BAD STATE"sv);
            /*
            log::warn(R"(RESUBSCRIBE symbol="{}")"sv, instrument.symbol);
            // XXX HANS publish stale
            collector.clear();
            shared_.depth_request_queue.emplace_back(instrument.symbol);
            */
          }
        })) {
    } else {
      // unknown instrument_id
      log::info<5>("DEBUG: unknown instrument_id={}"sv, instrument_id);
    }
  });
  // aggregator_.reset();  // XXX INCORRECT
}

void UDPEvents::operator()(Trace<deribit_multicast::Ticker> const &event, sbe::Frame const &frame) {
  auto &trace_info = event.trace_info;
  using value_type = std::remove_cvref<decltype(event)>::type::value_type;
  auto &ticker = const_cast<value_type &>(event.value);  // note! not const-safe
  log::info<4>("ticker={}, frame={}"sv, ticker, frame);
  auto &aggregator = get_aggregator(frame.channel_id);
  if (aggregator(frame.sequence_number)) {
    if (!publish_top_of_book_)
      return;
    auto const instrument_id = ticker.instrumentId();
    // note! skip previous updates
    // XXX NOT NECESSARY
    if (test_sequence(last_ticker_, instrument_id, frame.sequence_number)) {
      if (shared_.find_instrument(instrument_id, [&](auto &instrument) {
            std::chrono::milliseconds const timestamp{ticker.timestampMs()};
            // note! unlike the WS feed, it looks like we do *not* have to scale amounts here
            auto top_of_book = TopOfBook{
                .stream_id = stream_id_,
                .exchange = flags::Config::exchange(),
                .symbol = instrument.symbol,
                .layer{
                    .bid_price = ticker.bestBidPrice(),
                    .bid_quantity = ticker.bestBidAmount() * instrument.multiplier,
                    .ask_price = ticker.bestAskPrice(),
                    .ask_quantity = ticker.bestAskAmount() * instrument.multiplier,
                },
                .update_type = UpdateType::INCREMENTAL,
                .exchange_time_utc = timestamp,
                .exchange_sequence = {},
            };
            log::info<3>("top_of_book={}"sv, top_of_book);
            create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
          })) {
      } else {
        // unknown instrument_id
      }
    }
  }
}

void UDPEvents::operator()(Trace<deribit_multicast::Trades> const &event, sbe::Frame const &frame) {
  auto &trace_info = event.trace_info;
  using value_type = std::remove_cvref<decltype(event)>::type::value_type;
  auto &trades = const_cast<value_type &>(event.value);  // note! not const-safe
  log::info<4>("trades={}, frame={}"sv, trades, frame);
  auto &aggregator = get_aggregator(frame.channel_id);
  if (aggregator(frame.sequence_number)) {
    if (!publish_trade_summary_)
      return;
    auto const instrument_id = trades.instrumentId();
    // note! skip previous updates
    // XXX NOT NECESSARY
    if (test_sequence(last_trades_, instrument_id, frame.sequence_number)) {
      if (shared_.find_instrument(instrument_id, [&](auto &instrument) {
            std::chrono::milliseconds exchange_time_utc{};
            shared_.trades.clear();
            auto emplace_back = [](auto &result, auto multiplier, auto &value) {
              auto trade = Trade{
                  .side = sbe::map_direction(value.direction()),
                  .price = value.price(),
                  .quantity = value.amount() * multiplier,
                  .trade_id = {},
                  .taker_order_id = {},
                  .maker_order_id = {},
              };
              core::charconv::to_string(std::back_inserter(trade.trade_id), value.tradeId());
              result.emplace_back(std::move(trade));
            };
            trades.sbeRewind();
            trades.tradesList().forEach([&](auto const &item) {
              std::chrono::milliseconds const timestamp{item.timestampMs()};
              exchange_time_utc = std::max(exchange_time_utc, timestamp);
              emplace_back(shared_.trades, instrument.multiplier, item);
            });
            auto trade_summary = TradeSummary{
                .stream_id = stream_id_,
                .exchange = flags::Config::exchange(),
                .symbol = instrument.symbol,
                .trades = shared_.trades,
                .exchange_time_utc = exchange_time_utc,
                .exchange_sequence = {},
            };
            log::info<3>("trade_summary={}"sv, trade_summary);
            create_trace_and_dispatch(handler_, trace_info, trade_summary, true);
          })) {
      } else {
        // unknown instrument_id
      }
    }
  }
}

void UDPEvents::operator()(Trace<deribit_multicast::Snapshot> const &event, sbe::Frame const &frame) {
  using value_type = std::remove_cvref<decltype(event)>::type::value_type;
  auto &snapshot = const_cast<value_type &>(event.value);  // note! not const-safe
  log::info<4>("snapshot={}, frame={}"sv, snapshot, frame);
  log::fatal("Unexpected"sv);
}

void UDPEvents::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE);
}

void UDPEvents::publish_stream_status(TraceInfo const &trace_info, ConnectionStatus connection_status) {
  if (!utils::update(connection_status_, connection_status))
    return;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = {},
      .supports = supports_,
      .transport = Transport::UDP,
      .protocol = Protocol::SBE,
      .encoding = {Encoding::SBE},
      .priority = Priority::PRIMARY,
      .connection_status = connection_status_,
  };
  log::info("stream_status={}"sv, stream_status);
  create_trace_and_dispatch(handler_, trace_info, stream_status);
}

template <typename T, typename U>
void UDPEvents::emplace_back(T const &item, double multiplier, U &bids, U &asks) {
  auto mbp_update = MBPUpdate{
      .price = item.price(),
      .quantity = item.amount() * multiplier,
      .implied_quantity = NaN,
      .number_of_orders = {},
      .update_action = {},
      .price_level = {},
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

Aggregator &UDPEvents::get_aggregator(uint16_t channel_id) {
  auto iter = aggregator_.find(channel_id);
  if (iter == std::end(aggregator_))
    iter = aggregator_.emplace(channel_id, server::Flags::cache_mbp_max_depth()).first;
  return (*iter).second;
}

}  // namespace deribit
}  // namespace roq
