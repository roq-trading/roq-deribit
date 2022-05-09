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
    virtual void operator()(const Trace<StreamStatus const> &) = 0;
    virtual void operator()(const Trace<TopOfBook const> &, bool is_last) = 0;
    virtual void operator()(
        const Trace<MarketByPriceUpdate const> &, bool is_last, bool refresh) = 0;
    virtual void operator()(const Trace<TradeSummary const> &, bool is_last) = 0;
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
  void operator()(const Trace<deribit_multicast::Instrument> &, const sbe::Frame &) override;
  void operator()(const Trace<deribit_multicast::Book> &, const sbe::Frame &) override;
  void operator()(const Trace<deribit_multicast::Quote> &, const sbe::Frame &) override;
  void operator()(const Trace<deribit_multicast::Trades> &, const sbe::Frame &) override;
  // snapshot
  void operator()(const Trace<deribit_multicast::Snapshot> &, const sbe::Frame &) override;

  void publish_stream_status(const TraceInfo &);

  // events
  bool events_next_in_sequence(const sbe::Frame &);

  void reset_events();

  // snapshot
  bool snapshot_next_in_sequence(const sbe::Frame &);

  void publish_snapshot(
      const TraceInfo &,
      const std::string_view &symbol,
      std::chrono::nanoseconds exchange_time_utc,
      uint64_t exchange_sequence);

  void reset_snapshot();

  // utils
  template <typename T, typename U>
  static void emplace_back(const T &item, U &bids, U &asks);

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
  // events
  struct events_state_t {
    uint32_t previous_sequence_number_ = {};
    uint32_t previous_instrument_id_ = {};
    uint64_t previous_change_id_ = {};
    uint32_t skip_instrument_id_ = {};
    uint64_t skip_change_id_ = {};
    // -- note! required here because book updates may span multiple packets
    core::page_aligned_vector<MBPUpdate> bids_, asks_;
  } events_state_ = {};
  // snapshot
  struct snapshot_state_t {
    uint32_t previous_sequence_number_ = {};
    uint32_t previous_instrument_id_ = {};
    uint64_t previous_change_id_ = {};
    uint32_t skip_instrument_id_ = {};
    uint64_t skip_change_id_ = {};
    // -- note! required here because updates may span multiple packets
    core::page_aligned_vector<MBPUpdate> bids_, asks_;
  } snapshot_state_ = {};
};

}  // namespace deribit
}  // namespace roq
