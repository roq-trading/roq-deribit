/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <vector>

#include "roq/core/download.hpp"

#include "roq/core/stack/buffer.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/net/manager.hpp"
#include "roq/core/net/tcp_connection_factory.hpp"

#include "roq/server.hpp"

#include "roq/deribit/market_data_state.hpp"
#include "roq/deribit/security.hpp"
#include "roq/deribit/shared.hpp"

// session
#include "roq/deribit/fix/heartbeat.hpp"
#include "roq/deribit/fix/logon.hpp"
#include "roq/deribit/fix/logout.hpp"
#include "roq/deribit/fix/resend_request.hpp"
#include "roq/deribit/fix/test_request.hpp"

// business (inbound)
#include "roq/deribit/fix/market_data_incremental_refresh.hpp"
#include "roq/deribit/fix/market_data_request_reject.hpp"
#include "roq/deribit/fix/market_data_snapshot_full_refresh.hpp"
#include "roq/deribit/fix/security_list.hpp"
#include "roq/deribit/fix/security_status.hpp"

// business (outbound)
#include "roq/deribit/fix/market_data_request.hpp"
#include "roq/deribit/fix/security_list_request.hpp"
#include "roq/deribit/fix/security_status_request.hpp"

namespace roq {
namespace deribit {

class MarketData final : public core::net::Manager::Handler {
 public:
  struct SymbolsUpdate final {
    std::vector<Symbol> &symbols;
  };

  struct Handler {
    virtual void operator()(const Trace<StreamStatus> &) = 0;
    virtual void operator()(const Trace<ExternalLatency> &) = 0;
    virtual void operator()(const Trace<ReferenceData> &, bool is_last) = 0;
    virtual void operator()(
        const Trace<MarketByPriceUpdate> &, bool is_last, bool refresh) = 0;
    virtual void operator()(const Trace<TradeSummary> &, bool is_last) = 0;
    virtual void operator()(const Trace<StatisticsUpdate> &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  MarketData(
      Handler &,
      core::io::Context &,
      uint16_t stream_id,
      Security &,
      Shared &,
      size_t index,
      bool master);

  MarketData(const MarketData &) = delete;
  MarketData(MarketData &&) = delete;

  bool ready() const { return ready_; }

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  void subscribe(size_t start_from = 0);

  void operator()(const core::fix::Event<fix::Heartbeat> &, const TraceInfo &);
  void operator()(const core::fix::Event<fix::Logon> &, const TraceInfo &);
  void operator()(const core::fix::Event<fix::Logout> &, const TraceInfo &);
  void operator()(const core::fix::Event<fix::ResendRequest> &, const TraceInfo &);
  void operator()(const core::fix::Event<fix::TestRequest> &, const TraceInfo &);

  void operator()(const core::fix::Event<fix::SecurityList> &, const TraceInfo &);
  void operator()(const core::fix::Event<fix::SecurityStatus> &, const TraceInfo &);

  void operator()(
      const core::fix::Event<fix::MarketDataIncrementalRefresh> &, const TraceInfo &);
  void operator()(
      const core::fix::Event<fix::MarketDataRequestReject> &, const TraceInfo &);
  void operator()(
      const core::fix::Event<fix::MarketDataSnapshotFullRefresh> &, const TraceInfo &);

 protected:
  void operator()(const core::net::Manager::Connected &) override;
  void operator()(const core::net::Manager::Disconnected &) override;
  void operator()(const core::net::Manager::Read &) override;

 private:
  void operator()(ConnectionStatus);

  void send_logon();
  void send_logout(const std::string_view &text);
  void send_heartbeat(const std::string_view &test_req_id);
  void send_test_request(std::chrono::nanoseconds now);

  uint32_t download(MarketDataState);

  void download_securities();

  void subscribe(const std::span<Symbol const> &symbols);
  void unsubscribe(const std::span<Symbol const> &symbols);

  void resubscribe(const std::string_view &symbol);

  void parse(const core::fix::message_t &);
  void parse_helper(const core::fix::message_t &);

  // utilities

  template <typename T>
  void send(const T &event);

  template <typename T>
  void send(const T &event, std::chrono::nanoseconds sending_time);

  void check(const core::fix::header_t &);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const size_t index_;
  const bool master_;
  // connection
  core::net::TcpConnectionFactory connection_factory_;
  core::net::Manager connection_;
  // buffers
  core::Buffer encode_buffer_;
  core::Buffer decode_buffer_;
  core::stack::Buffer<char, 32> stack_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, security_list, security_status, market_data_incremental_refresh,
        market_data_request_reject, market_data_snapshot_full_refresh, market_data_request;
  } profile_;
  struct {
    core::metrics::Latency ping;
  } latency_;
  // state
  struct {
    uint64_t msg_seq_num = {};
  } outbound_;
  struct {
    uint64_t msg_seq_num = {};
  } inbound_;
  // security
  Security &security_;
  // cache
  Shared &shared_;
  // state
  bool ready_ = false;
  std::chrono::nanoseconds next_heartbeat_ = {};
  ConnectionStatus status_ = {};
  core::Download<MarketDataState> download_;
  std::chrono::nanoseconds last_logon_or_heartbeat_ = {};
  absl::flat_hash_set<Symbol> latch_;
};

}  // namespace deribit
}  // namespace roq
