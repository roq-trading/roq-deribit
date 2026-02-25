/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/udp_events.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/charconv/to_string.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/utils/debug/hex/message.hpp"

#include "roq/io/network_address.hpp"

#include "roq/deribit/utils.hpp"

#include "roq/deribit/sbe/map.hpp"
#include "roq/deribit/sbe/utils.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const NAME = "udpe"sv;
}

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto publish_top_of_book(auto &settings) {
  return !settings.multicast.disable_top_of_book;
}

auto publish_market_by_price(auto &settings) {
  return !settings.multicast.disable_market_by_price;
}

auto publish_trade_summary(auto &settings) {
  return !settings.multicast.disable_trade_summary;
}

auto get_supports(auto publish_top_of_book, auto publish_market_by_price, auto publish_trade_summary) {
  Mask<SupportType> result;
  if (publish_top_of_book) {
    result |= SupportType::TOP_OF_BOOK;
  }
  if (publish_market_by_price) {
    result |= SupportType::MARKET_BY_PRICE;
  }
  if (publish_trade_summary) {
    result |= SupportType::TRADE_SUMMARY;
  }
  return result;
}

auto create_receiver(auto &handler, auto &settings, auto &context, auto &shared) {
  auto port = shared.sbe_config.events_port();
  log::info("Create multicast socket port={}"sv, port);
  auto network_address = io::NetworkAddress{port};
  auto socket_options = Mask{
      io::SocketOption::REUSE_ADDRESS,
  };
  auto receiver = context.create_udp_receiver(handler, network_address, socket_options);
  log::info(R"(Local interface is "{}")"sv, settings.multicast.local_interface);
  auto local_interface = io::NetworkAddress::create_blocking(settings.multicast.local_interface);
  auto callback = [&](auto &connection) {
    log::info(R"(Add membership "{}")"sv, connection.address);
    auto multicast_address_2 = io::NetworkAddress::create_blocking(connection.address);
    (*receiver).add_membership(multicast_address_2, local_interface);
  };
  shared.sbe_config.get_connections(callback);
  return receiver;
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};

// following is used from several places

bool test_sequence(auto &cache, auto instrument_id, auto sequence_number) {
  auto result = false;
  constexpr uint32_t const midpoint = 1 << 31;
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
  if (result) {
    (*iter).second = sequence_number;
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

UDPEvents::UDPEvents(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, publish_top_of_book_{publish_top_of_book(shared.settings)},
      publish_market_by_price_{publish_market_by_price(shared.settings)}, publish_trade_summary_{publish_trade_summary(shared.settings)},
      supports_{get_supports(publish_top_of_book_, publish_market_by_price_, publish_trade_summary_)},
      receiver_{create_receiver(*this, shared.settings, context, shared)},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
      },
      shared_{shared} {
  log::info("publish_top_of_book={}"sv, publish_top_of_book_);
  log::info("publish_market_by_price={}"sv, publish_market_by_price_);
  log::info("publish_trade_summary={}"sv, publish_trade_summary_);
}

void UDPEvents::operator()(Event<Start> const &) {
  TraceInfo trace_info;
  publish_stream_status(trace_info, ConnectionStatus::CONNECTING);
  last_update_time_ = trace_info.source_receive_time;
}

void UDPEvents::operator()(Event<Stop> const &) {
}

void UDPEvents::operator()(Event<Timer> const &event) {
  if (last_update_time_.count() != 0 && (last_update_time_ + shared_.settings.multicast.timeout) < event.value.now) {
    log::warn("*** DETECTED TIMEOUT ***"sv);
    last_update_time_ = {};
  }
}

void UDPEvents::operator()(io::net::udp::Receiver::Read const &) {
  TraceInfo trace_info;
  last_update_time_ = trace_info.source_receive_time;
  publish_stream_status(trace_info, ConnectionStatus::READY);  // first message will publish
  while (true) {
    auto bytes = (*receiver_).recv(shared_.buffer);
    if (bytes == 0) {
      break;
    }
    log::info<5>("Received {} byte(s)"sv, bytes);
    std::span payload{std::data(shared_.buffer), bytes};
    log::info<5>("{}"sv, utils::debug::hex::Message{payload});
    if (!sbe::Parser::dispatch(*this, payload, trace_info)) {
      // note! here is an option to use the re-order buffer -- but it's not A+B, so why bother?
    }
  }
}

void UDPEvents::operator()(io::net::udp::Receiver::Error const &error) {
  log::fatal("Error: what={}"sv, error.what);
}

bool UDPEvents::operator()(sbe::Frame const &frame) {
  auto result = false;
  auto callback = [&](auto &channel) {
    result = channel(frame);
    if (!result) {
      channel.reset(frame);
    }
  };
  get_channel(frame, callback);
  return result;
}

void UDPEvents::operator()(Trace<deribit_multicast::Instrument> const &event, sbe::Frame const &frame) {
  using value_type = std::remove_cvref_t<decltype(event)>::value_type;
  auto &instrument = const_cast<value_type &>(event.value);
  log::info<2>("instrument={}, frame={}"sv, instrument, frame);
  auto instrument_id = instrument.instrumentId();
  auto callback = [&]() -> Instrument {
    auto contract_size = instrument.contractSize();
    auto multiplier = compute_contracts_multiplier(contract_size);
    auto symbol = sbe::get_instrument_name(instrument);  // note! must be **LAST***
    auto discard = shared_.discard_symbol(symbol);
    if (!discard) {
      log::debug(R"(CREATE instrument_id={}, instrument_name="{}", contract_size={}, multiplier={})"sv, instrument_id, symbol, contract_size, multiplier);
    }
    return {
        symbol,
        contract_size,
        multiplier,
        discard,
    };
  };
  shared_.maybe_create_instrument(instrument_id, callback);
}

// note! it may take a while before the instrument gets created (depends on the snapshot channel)
void UDPEvents::operator()(Trace<deribit_multicast::Book> const &event, sbe::Frame const &frame) {
  auto &trace_info = event.trace_info;
  using value_type = std::remove_cvref_t<decltype(event)>::value_type;
  auto &book = const_cast<value_type &>(event.value);
  log::info<4>("book={}, frame={}"sv, book, frame);
  if (!publish_market_by_price_) {
    return;
  }
  auto instrument_id = book.instrumentId();
  auto prev_change_id = book.prevChangeId();
  auto change_id = book.changeId();
  auto is_last = book.isLast();
  auto callback = [&](auto &channel) {
    auto callback_2 = [&](auto &instrument) {
      std::chrono::milliseconds timestamp{book.timestampMs()};
      auto append_change = [&](auto &item) {
        auto price = item.price();
        auto quantity = item.amount() * instrument.multiplier;
        auto mbp_update = MBPUpdate{
            .price = price,
            .quantity = quantity,
            .implied_quantity = NaN,
            .number_of_orders = {},
            .update_action = {},
            .price_level = {},
        };
        auto side = map(deribit_multicast::BookSide::get(item.side())).template get<Side>();
        switch (side) {
          case Side::UNDEFINED:
            assert(false);
            break;
          case Side::BUY:
            channel.bids.emplace_back(std::move(mbp_update));
            break;
          case Side::SELL:
            channel.asks.emplace_back(std::move(mbp_update));
            break;
        }
      };
      book.sbeRewind();
      book.changesList().forEach(append_change);
      if (!is_last) {
        if (!channel.instrument_id) {
          channel.instrument_id = instrument_id;
        } else {
          assert(channel.instrument_id == instrument_id);
        }
        return;
      }
      try {
        auto create_update = [&](auto &bids, auto &asks, auto update_type, auto exchange_sequence) -> MarketByPriceUpdate {
          return {
              .stream_id = stream_id_,
              .exchange = shared_.settings.exchange,
              .symbol = instrument.symbol,
              .bids = bids,
              .asks = asks,
              .update_type = update_type,
              .exchange_time_utc = timestamp,
              .exchange_sequence = exchange_sequence,
              .sending_time_utc = {},
              .price_precision = {},
              .quantity_precision = {},
              .checksum = {},
          };
        };
        auto publish_update = [&](auto &bids, auto &asks) {
          if (std::empty(bids) && std::empty(asks)) {  // note! empty updates are common
            return;
          }
          auto market_by_price_update = create_update(bids, asks, UpdateType::INCREMENTAL, change_id);
          auto is_last = true;
          create_trace_and_dispatch(handler_, trace_info, market_by_price_update, is_last);
        };
        auto publish_snapshot = [&](auto &bids, auto &asks, auto exchange_sequence, auto retries, auto delay) {
          log::info(
              R"(PUBLISH MBP SNAPSHOT exchange="{}", symbol="{}", instrument_id={}, exchange_sequence={}, retries={}, delay={})"sv,
              shared_.settings.exchange,
              instrument.symbol,
              instrument_id,
              exchange_sequence,
              retries,
              std::chrono::duration_cast<std::chrono::milliseconds>(delay));
          auto market_by_price_update = create_update(bids, asks, UpdateType::SNAPSHOT, exchange_sequence);
          auto apply_updates = [&](auto &market_by_price) {
            auto include = true;
            instrument.mbp_sequencer.apply(market_by_price, exchange_sequence, include);
          };
          Trace event{trace_info, market_by_price_update};
          shared_(event, true, apply_updates);
        };
        auto request_snapshot = [&](auto retries) {
          log::info(
              R"(REQUEST MBP SNAPSHOT exchange="{}", symbol="{}", instrument_id={}, change_id={}, retries={})"sv,
              shared_.settings.exchange,
              instrument.symbol,
              instrument_id,
              change_id,
              retries);
          // note! just wait for snapshot
        };
        instrument.mbp_sequencer(channel.bids, channel.asks, change_id, change_id, prev_change_id, publish_update, publish_snapshot, request_snapshot);
      } catch (BadState &) {
        log::warn(
            R"(RESUBSCRIBE MBP exchange="{}", symbol="{}", security_id={}, change_id={})"sv,
            shared_.settings.exchange,
            instrument.symbol,
            instrument_id,
            change_id);
        // XXX HANS publish stale
        instrument.mbp_sequencer.clear();
      }
      channel.instrument_id = {};
      channel.bids.clear();
      channel.asks.clear();
    };
    if (shared_.find_instrument(instrument_id, callback_2)) {
    } else {
      // log::debug("Unexpected: unknown instrument_id={}"sv, instrument_id);
      log::info<5>("Unexpected: unknown instrument_id={}"sv, instrument_id);
    }
  };
  get_channel(frame, callback);
}

void UDPEvents::operator()(Trace<deribit_multicast::Trades> const &event, sbe::Frame const &frame) {
  auto &trace_info = event.trace_info;
  using value_type = std::remove_cvref_t<decltype(event)>::value_type;
  auto &trades = const_cast<value_type &>(event.value);
  log::info<4>("trades={}, frame={}"sv, trades, frame);
  if (!publish_trade_summary_) {
    return;
  }
  auto instrument_id = trades.instrumentId();
  auto callback = [&](auto &instrument) {
    std::chrono::milliseconds exchange_time_utc = {};
    auto &trades_2 = shared_.get_trades();
    auto append_trade = [&](auto &item) {
      std::chrono::milliseconds timestamp{item.timestampMs()};
      exchange_time_utc = std::max(exchange_time_utc, timestamp);
      auto price = item.price();
      auto quantity = item.amount() * instrument.multiplier;
      auto trade = Trade{
          .side = map(item.direction()),
          .price = price,
          .quantity = quantity,
          .trade_id = {},
          .taker_order_id = {},
          .maker_order_id = {},
      };
      utils::charconv::to_string(std::back_inserter(trade.trade_id), item.tradeId());
      trades_2.emplace_back(trade);  // XXX FIXME std::move
    };
    trades.sbeRewind();
    trades.tradesList().forEach(append_trade);
    auto trade_summary = TradeSummary{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = instrument.symbol,
        .trades = trades_2,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    log::info<3>("trade_summary={}"sv, trade_summary);
    auto is_last = true;
    create_trace_and_dispatch(handler_, trace_info, trade_summary, is_last);
  };
  if (test_sequence(last_trades_, instrument_id, frame.sequence_number)) {
    if (shared_.find_instrument(instrument_id, callback)) {
    } else {
      // XXX unknown instrument_id
    }
  }
}

void UDPEvents::operator()(Trace<deribit_multicast::Ticker> const &event, sbe::Frame const &frame) {
  auto &trace_info = event.trace_info;
  using value_type = std::remove_cvref_t<decltype(event)>::value_type;
  auto &ticker = const_cast<value_type &>(event.value);
  log::info<4>("ticker={}, frame={}"sv, ticker, frame);
  if (!publish_top_of_book_) {
    return;
  }
  auto instrument_id = ticker.instrumentId();
  auto callback = [&](auto &instrument) {
    auto bid_price = ticker.bestBidPrice();
    auto bid_quantity = ticker.bestBidAmount() * instrument.multiplier;
    auto ask_price = ticker.bestAskPrice();
    auto ask_quantity = ticker.bestAskAmount() * instrument.multiplier;
    std::chrono::milliseconds timestamp{ticker.timestampMs()};
    auto top_of_book = TopOfBook{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = instrument.symbol,
        .layer{
            .bid_price = bid_price,
            .bid_quantity = bid_quantity,
            .ask_price = ask_price,
            .ask_quantity = ask_quantity,
        },
        .update_type = UpdateType::SNAPSHOT,  // XXX FIXME ???
        .exchange_time_utc = timestamp,
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    log::info<3>("top_of_book={}"sv, top_of_book);
    auto is_last = true;
    create_trace_and_dispatch(handler_, trace_info, top_of_book, is_last);
  };
  if (test_sequence(last_ticker_, instrument_id, frame.sequence_number)) {
    if (shared_.find_instrument(instrument_id, callback)) {
    } else {
      // XXX unknown instrument_id
    }
  }
}

void UDPEvents::operator()(Trace<deribit_multicast::Snapshot> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPEvents::operator()(Trace<deribit_multicast::SnapshotStart> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPEvents::operator()(Trace<deribit_multicast::SnapshotEnd> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPEvents::operator()(Trace<deribit_multicast::ComboLegs> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPEvents::operator()(Trace<deribit_multicast::PriceIndex> const &, sbe::Frame const &) {
  // XXX FIXME TODO
}

void UDPEvents::operator()(Trace<deribit_multicast::Rfq> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPEvents::operator()(Trace<deribit_multicast::InstrumentV2> const &, sbe::Frame const &) {
  // XXX FIXME can't make get_instrument_name() to work...
}

void UDPEvents::operator()(metrics::Writer &writer) const {
  writer  //
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(profile_.parse, metrics::Type::PROFILE);
}

void UDPEvents::publish_stream_status(TraceInfo const &trace_info, ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = {},
      .supports = supports_,
      .transport = Transport::UDP,
      .protocol = Protocol::SBE,
      .encoding = {Encoding::SBE},
      .priority = Priority::PRIMARY,
      .connection_status = connection_status_,
      .reason = reason,
      .interface = {},
      .authority = {},
      .path = {},
      .proxy = {},
  };
  log::info("stream_status={}"sv, stream_status);
  create_trace_and_dispatch(handler_, trace_info, stream_status);
}

template <typename Callback>
void UDPEvents::get_channel(sbe::Frame const &frame, Callback callback) {
  auto iter = channel_.find(frame.channel_id);
  if (iter == std::end(channel_)) {
    iter = channel_.try_emplace(frame.channel_id).first;
  }
  callback((*iter).second);
}

}  // namespace deribit
}  // namespace roq
