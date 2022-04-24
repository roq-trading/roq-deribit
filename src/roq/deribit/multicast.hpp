/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/net/udp_connection.hpp"

#include "roq/server.hpp"

#include "roq/deribit/shared.hpp"

#include "roq/deribit/sbe/parser.hpp"

namespace roq {
namespace deribit {

class Multicast final : public core::net::UdpConnection::Handler, public sbe::Parser::Handler {
 public:
  struct Handler {
    virtual void operator()(const Trace<StreamStatus> &) = 0;
    virtual void operator()(const Trace<TopOfBook> &, bool is_last) = 0;
    virtual void operator()(const Trace<MarketByPriceUpdate> &, bool is_last, bool refresh) = 0;
    virtual void operator()(const Trace<TradeSummary> &, bool is_last) = 0;
  };

  Multicast(Handler &, core::io::Context &, uint16_t stream_id, Shared &);

  Multicast(const Multicast &) = delete;
  Multicast(Multicast &&) = delete;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

 protected:
  void operator()(const core::net::UdpConnection::Read &) override;
  void operator()(const core::net::UdpConnection::Error &) override;

 protected:
  // events
  void operator()(const Trace2<deribit_multicast::Instrument> &, const sbe::Frame &) override;
  void operator()(const Trace2<deribit_multicast::Book> &, const sbe::Frame &) override;
  void operator()(const Trace2<deribit_multicast::Quote> &, const sbe::Frame &) override;
  void operator()(const Trace2<deribit_multicast::Trades> &, const sbe::Frame &) override;
  // snapshot
  void operator()(const Trace2<deribit_multicast::Snapshot> &, const sbe::Frame &) override;

  void publish_stream_status(const TraceInfo &);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const bool publish_top_of_book_;
  const bool publish_market_by_price_;
  const bool publish_trade_summary_;
  // connection
  core::net::UdpConnection events_;
  core::net::UdpConnection snapshot_;
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
  absl::flat_hash_map<uint32_t, uint32_t> last_quote_;
  absl::flat_hash_map<uint32_t, uint32_t> last_trades_;
};

}  // namespace deribit
}  // namespace roq
