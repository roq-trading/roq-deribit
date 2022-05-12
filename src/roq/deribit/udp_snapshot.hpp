/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/net/udp_connection.hpp"

#include "roq/server.hpp"

#include "roq/deribit/aggregator.hpp"
#include "roq/deribit/shared.hpp"

#include "roq/deribit/sbe/parser.hpp"

namespace roq {
namespace deribit {

class UDPSnapshot final : public core::net::UdpConnection::Handler, public sbe::Parser::Handler {
 public:
  struct Handler {
    virtual void operator()(const Trace<StreamStatus const> &) = 0;
    virtual void operator()(
        const Trace<MarketByPriceUpdate const> &, bool is_last, bool refresh) = 0;
  };

  UDPSnapshot(Handler &, core::io::Context &, uint16_t stream_id, Shared &);

  UDPSnapshot(const UDPSnapshot &) = delete;
  UDPSnapshot(UDPSnapshot &&) = delete;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

 protected:
  void operator()(const core::net::UdpConnection::Read &) override;
  void operator()(const core::net::UdpConnection::Error &) override;

 protected:
  // events
  void operator()(const Trace<deribit_multicast::Instrument> &, const sbe::Frame &) override;
  void operator()(const Trace<deribit_multicast::Book> &, const sbe::Frame &) override;
  void operator()(const Trace<deribit_multicast::Quote> &, const sbe::Frame &) override;
  void operator()(const Trace<deribit_multicast::Trades> &, const sbe::Frame &) override;
  // snapshot
  void operator()(const Trace<deribit_multicast::Snapshot> &, const sbe::Frame &) override;

  void publish_stream_status(const TraceInfo &);

  // utils
  template <typename T, typename U>
  static void emplace_back(const T &item, U &bids, U &asks);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const bool publish_market_by_price_;
  // connection
  core::net::UdpConnection connection_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse;
  } profile_;
  // cache
  Shared &shared_;
  bool initialized_ = false;
  Aggregator aggregator_;
};

}  // namespace deribit
}  // namespace roq
