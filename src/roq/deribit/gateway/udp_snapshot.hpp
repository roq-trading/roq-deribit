/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/utils/container.hpp"

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/io/net/udp/receiver.hpp"

#include "roq/server.hpp"

#include "roq/deribit/gateway/channel.hpp"
#include "roq/deribit/gateway/shared.hpp"

#include "roq/deribit/protocol/sbe/parser.hpp"

namespace roq {
namespace deribit {
namespace gateway {

struct UDPSnapshot final : public io::net::udp::Receiver::Handler, public protocol::sbe::Parser::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<MarketByPriceUpdate> const &, bool is_last) = 0;
  };

  UDPSnapshot(Handler &, io::Context &, uint16_t stream_id, Shared &);

  UDPSnapshot(UDPSnapshot const &) = delete;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

 protected:
  // io::net::udp::Receiver::Handler
  void operator()(io::net::udp::Receiver::Read const &) override;
  void operator()(io::net::udp::Receiver::Error const &) override;

  // protocol::sbe::Parser::Handler
  bool operator()(protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::Instrument> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::Book> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::Trades> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::Ticker> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::Snapshot> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::SnapshotStart> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::SnapshotEnd> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::ComboLegs> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::PriceIndex> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::Rfq> const &, protocol::sbe::Frame const &) override;
  void operator()(Trace<::deribit::sbe::multicast::InstrumentV2> const &, protocol::sbe::Frame const &) override;

  // utils

  void publish_stream_status(TraceInfo const &, ConnectionStatus, std::string_view const &reason = {});

  template <typename Callback>
  void get_channel(protocol::sbe::Frame const &, Callback);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  bool const publish_market_by_price_;
  Mask<SupportType> const supports_;
  // receiver
  std::unique_ptr<io::net::udp::Receiver> const receiver_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse;
  } profile_;
  // cache
  Shared &shared_;
  ConnectionStatus connection_status_ = {};
  utils::unordered_map<uint16_t, Channel> channel_;
  // state
  std::chrono::nanoseconds last_update_time_ = {};
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
