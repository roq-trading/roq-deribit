/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/utils/container.hpp"

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/io/net/udp/receiver.hpp"

#include "roq/server.hpp"

#include "roq/deribit/channel.hpp"
#include "roq/deribit/shared.hpp"

#include "roq/deribit/sbe/parser.hpp"

namespace roq {
namespace deribit {

struct UDPSnapshot final : public io::net::udp::Receiver::Handler, public sbe::Parser::Handler {
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

  // sbe::Parser::Handler
  bool operator()(sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::Instrument> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::Book> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::Trades> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::Ticker> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::Snapshot> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::SnapshotStart> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::SnapshotEnd> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::ComboLegs> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::PriceIndex> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::Rfq> const &, sbe::Frame const &) override;
  void operator()(Trace<deribit_multicast::InstrumentV2> const &, sbe::Frame const &) override;

  // utils

  void publish_stream_status(TraceInfo const &, ConnectionStatus, std::string_view const &reason = {});

  template <typename Callback>
  void get_channel(sbe::Frame const &, Callback);

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

}  // namespace deribit
}  // namespace roq
