/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/udp_snapshot.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/debug/hex/message.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/io/network_address.hpp"

#include "roq/deribit/utils.hpp"

#include "roq/deribit/sbe/map.hpp"
#include "roq/deribit/sbe/utils.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const NAME = "udps"sv;
}

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto publish_market_by_price(auto &settings) {
  return !settings.multicast.disable_market_by_price;
}

auto get_supports(auto publish_market_by_price) {
  Mask<SupportType> result;
  if (publish_market_by_price) {
    result |= SupportType::MARKET_BY_PRICE;
  }
  return result;
}

auto create_receiver(auto &handler, auto &settings, auto &context, auto &shared) {
  auto port = shared.sbe_config.snapshot_port();
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
}  // namespace

// === IMPLEMENTATION ===

UDPSnapshot::UDPSnapshot(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, publish_market_by_price_{publish_market_by_price(shared.settings)},
      supports_{get_supports(publish_market_by_price_)}, receiver_{create_receiver(*this, shared.settings, context, shared)},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
      },
      shared_{shared} {
  log::info("publish_market_by_price={}"sv, publish_market_by_price_);
}

void UDPSnapshot::operator()(Event<Start> const &) {
  TraceInfo trace_info;
  last_update_time_ = trace_info.source_receive_time;
  publish_stream_status(trace_info, ConnectionStatus::CONNECTING);
}

void UDPSnapshot::operator()(Event<Stop> const &) {
}

void UDPSnapshot::operator()(Event<Timer> const &event) {
  if (last_update_time_.count() != 0 && (last_update_time_ + shared_.settings.multicast.timeout) < event.value.now) {
    log::warn("*** DETECTED TIMEOUT ***"sv);
    last_update_time_ = {};
  }
}

void UDPSnapshot::operator()(io::net::udp::Receiver::Read const &) {
  TraceInfo trace_info;
  last_update_time_ = trace_info.source_receive_time;
  publish_stream_status(trace_info, ConnectionStatus::READY);  // first message will publish
  while (true) {
    auto bytes = (*receiver_).recv(shared_.buffer);
    if (bytes == 0) {
      return;
    }
    log::info<5>("Received {} byte(s)"sv, bytes);
    std::span payload{std::data(shared_.buffer), bytes};
    log::info<5>("{}"sv, utils::debug::hex::Message{payload});
    if (!sbe::Parser::dispatch(*this, payload, trace_info)) {
      // note! here is an option to use the re-order buffer -- but it's not A+B, so why bother?
    }
  }
}

void UDPSnapshot::operator()(io::net::udp::Receiver::Error const &error) {
  log::fatal("Error: what={}"sv, error.what);
}

bool UDPSnapshot::operator()(sbe::Frame const &frame) {
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

void UDPSnapshot::operator()(Trace<deribit_multicast::Instrument> const &event, sbe::Frame const &frame) {
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

void UDPSnapshot::operator()(Trace<deribit_multicast::Book> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::Trades> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::Ticker> const &event, sbe::Frame const &frame) {
  using value_type = std::remove_cvref_t<decltype(event)>::value_type;
  auto &ticker = const_cast<value_type &>(event.value);
  log::info<4>("ticker={}, frame={}"sv, ticker, frame);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::Snapshot> const &event, sbe::Frame const &frame) {
  auto &trace_info = event.trace_info;
  using value_type = std::remove_cvref_t<decltype(event)>::value_type;
  auto &snapshot = const_cast<value_type &>(event.value);
  log::info<4>("snapshot={}, frame={}"sv, snapshot, frame);
  if (!publish_market_by_price_) {
    return;
  }
  auto instrument_id = snapshot.instrumentId();
  auto change_id = snapshot.changeId();
  auto is_last = snapshot.isLastInBook();
  auto callback = [&](auto &channel) {
    auto callback_2 = [&](auto &instrument) {
      // XXX FIXME there is a race if udp_events resets in the middle of receiving snapshot
      if (channel.ready() && !instrument.mbp_sequencer.ready()) {
        auto append_level = [&](auto &item) {
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
        snapshot.sbeRewind();
        snapshot.levelsList().forEach(append_level);
      }
      if (!is_last) {
        if (!channel.instrument_id) {
          channel.instrument_id = instrument_id;
        } else {
          assert(channel.instrument_id == instrument_id);
        }
        return;
      }
      if (channel.ready() && !(std::empty(channel.bids) && std::empty(channel.asks))) {
        auto publish_snapshot = [&](auto &bids, auto &asks, auto exchange_sequence, auto retries, auto delay) {
          log::info(
              R"(PUBLISH MBP SNAPSHOT exchange="{}", symbol="{}", instrument_id={}, exchange_sequence={}, retries={}, delay={})"sv,
              shared_.settings.exchange,
              instrument.symbol,
              instrument_id,
              exchange_sequence,
              retries,
              std::chrono::duration_cast<std::chrono::milliseconds>(delay));
          std::chrono::milliseconds timestamp{snapshot.timestampMs()};
          auto market_by_price_update = MarketByPriceUpdate{
              .stream_id = stream_id_,
              .exchange = shared_.settings.exchange,
              .symbol = instrument.symbol,
              .bids = bids,
              .asks = asks,
              .update_type = UpdateType::SNAPSHOT,
              .exchange_time_utc = timestamp,
              .exchange_sequence = exchange_sequence,
              .sending_time_utc = {},
              .price_precision = {},
              .quantity_precision = {},
              .checksum = {},
          };
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
          // note! just wait for next snapshot
        };
        auto force = false;
        instrument.mbp_sequencer(channel.bids, channel.asks, change_id, force, publish_snapshot, request_snapshot);
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

void UDPSnapshot::operator()(Trace<deribit_multicast::SnapshotStart> const &, sbe::Frame const &frame) {
  auto callback = [&](auto &channel) { channel.snapshot_start(frame); };
  get_channel(frame, callback);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::SnapshotEnd> const &, sbe::Frame const &frame) {
  auto callback = [&](auto &channel) { channel.snapshot_end(frame); };
  get_channel(frame, callback);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::ComboLegs> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::PriceIndex> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::Rfq> const &, sbe::Frame const &) {
  log::fatal("Unexpected"sv);
}

void UDPSnapshot::operator()(Trace<deribit_multicast::InstrumentV2> const &, sbe::Frame const &) {
  // XXX FIXME can't make get_instrument_name() to work...
}

void UDPSnapshot::operator()(metrics::Writer &writer) const {
  writer  //
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(profile_.parse, metrics::Type::PROFILE);
}

void UDPSnapshot::publish_stream_status(TraceInfo const &trace_info, ConnectionStatus connection_status, std::string_view const &reason) {
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
void UDPSnapshot::get_channel(sbe::Frame const &frame, Callback callback) {
  auto iter = channel_.find(frame.channel_id);
  if (iter == std::end(channel_)) {
    iter = channel_.try_emplace(frame.channel_id).first;
  }
  callback((*iter).second);
}

}  // namespace deribit
}  // namespace roq
