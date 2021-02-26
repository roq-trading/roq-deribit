/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <string>
#include <vector>

#include "roq/core/stack/buffer.h"

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/net/manager.h"
#include "roq/core/net/tcp_connection_factory.h"

#include "roq/server.h"

#include "roq/deribit/security.h"

// session
#include "roq/deribit/fix/heartbeat.h"
#include "roq/deribit/fix/logon.h"
#include "roq/deribit/fix/logout.h"
#include "roq/deribit/fix/resend_request.h"
#include "roq/deribit/fix/test_request.h"

// business (inbound)
#include "roq/deribit/fix/market_data_incremental_refresh.h"
#include "roq/deribit/fix/market_data_request_reject.h"
#include "roq/deribit/fix/market_data_snapshot_full_refresh.h"

// business (outbound)
#include "roq/deribit/fix/market_data_request.h"

namespace roq {
namespace deribit {

class MarketData final : public core::net::Manager::Handler {
 public:
  struct Handler {
    virtual void operator()(const MarketData &) = 0;

    virtual void operator()(const ExternalLatency &, const server::TraceInfo &) = 0;

    virtual void operator()(
        const fix::MarketDataIncrementalRefresh &, const server::TraceInfo &) = 0;
    virtual void operator()(const fix::MarketDataRequestReject &, const server::TraceInfo &) = 0;
    virtual void operator()(
        const fix::MarketDataSnapshotFullRefresh &, const server::TraceInfo &) = 0;
  };

  MarketData(
      Handler &handler,
      Security &security,
      core::io::Context &context,
      uint32_t stream_id,
      std::vector<std::string> &&symbols);

  MarketData(const MarketData &) = delete;
  MarketData(MarketData &&) = delete;

  bool ready() const;

  void close();

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(const fix::MarketDataRequest &);

  void operator()(metrics::Writer &writer);

 protected:
  void operator()(const core::net::Manager::Connected &) override;
  void operator()(const core::net::Manager::Disconnected &) override;
  void operator()(const core::net::Manager::Read &) override;

 private:
  template <typename T>
  void send(const T &event);

  template <typename T>
  void send(const T &event, std::chrono::nanoseconds sending_time);

  void send_logon();
  void send_logout(const std::string_view &text);
  void send_heartbeat(const std::string_view &test_req_id);
  void send_test_request(std::chrono::nanoseconds now);

  void check(const core::fix::header_t &header);

  void parse(const core::fix::message_t &message);
  void parse_helper(const core::fix::message_t &message);

  void operator()(const core::fix::header_t &, const fix::Heartbeat &, const server::TraceInfo &);
  void operator()(const core::fix::header_t &, const fix::Logon &, const server::TraceInfo &);
  void operator()(const core::fix::header_t &, const fix::Logout &, const server::TraceInfo &);
  void operator()(
      const core::fix::header_t &, const fix::ResendRequest &, const server::TraceInfo &);
  void operator()(const core::fix::header_t &, const fix::TestRequest &, const server::TraceInfo &);

  void operator()(
      const core::fix::header_t &,
      const fix::MarketDataIncrementalRefresh &,
      const server::TraceInfo &);
  void operator()(
      const core::fix::header_t &, const fix::MarketDataRequestReject &, const server::TraceInfo &);
  void operator()(
      const core::fix::header_t &,
      const fix::MarketDataSnapshotFullRefresh &,
      const server::TraceInfo &);

  void subscribe();

 private:
  Handler &handler_;
  // config
  const uint32_t stream_id_;
  std::vector<std::string> symbols_;
  const std::string name_;
  // security
  Security &security_;
  // connection
  core::net::TcpConnectionFactory connection_factory_;
  core::net::Manager connection_;
  // buffers
  core::utils::Buffer encode_buffer_;
  core::utils::Buffer decode_buffer_;
  core::stack::Buffer<char, 32u> stack_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, market_data_incremental_refresh, market_data_request_reject,
        market_data_snapshot_full_refresh;
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
  bool ready_ = false;
  std::chrono::nanoseconds next_heartbeat_ = {};
};

}  // namespace deribit
}  // namespace roq
