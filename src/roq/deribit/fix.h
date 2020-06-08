/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string>

#include "roq/core/stack/buffer.h"

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/event/base.h"
#include "roq/core/event/dns_base.h"

#include "roq/core/net/manager.h"
#include "roq/core/net/tcp_connection_factory.h"

#include "roq/server.h"

#include "roq/deribit/config.h"
#include "roq/deribit/random.h"

// session
#include "roq/deribit/fix/heartbeat.h"
#include "roq/deribit/fix/logon.h"
#include "roq/deribit/fix/logout.h"
#include "roq/deribit/fix/resend_request.h"
#include "roq/deribit/fix/test_request.h"

// business (inbound)
#include "roq/deribit/fix/execution_report.h"
#include "roq/deribit/fix/market_data_incremental_refresh.h"
#include "roq/deribit/fix/market_data_request_reject.h"
#include "roq/deribit/fix/market_data_snapshot_full_refresh.h"
#include "roq/deribit/fix/order_cancel_reject.h"
#include "roq/deribit/fix/position_report.h"
#include "roq/deribit/fix/reject.h"  // ... normally session level
#include "roq/deribit/fix/security_list.h"
#include "roq/deribit/fix/security_status.h"
#include "roq/deribit/fix/user_response.h"

// business (outbound)
#include "roq/deribit/fix/market_data_request.h"
#include "roq/deribit/fix/new_order_single.h"
#include "roq/deribit/fix/order_cancel_replace_request.h"
#include "roq/deribit/fix/order_cancel_request.h"
#include "roq/deribit/fix/order_mass_status_request.h"
#include "roq/deribit/fix/request_for_positions.h"
#include "roq/deribit/fix/security_list_request.h"
#include "roq/deribit/fix/security_status_request.h"
#include "roq/deribit/fix/user_request.h"

namespace roq {
namespace deribit {

class FIX final : public core::net::Manager::Handler {
 public:
  struct Handler {
    virtual void operator()(const FIX&) = 0;
    virtual void operator()(const fix::ExecutionReport&) = 0;
    virtual void operator()(const fix::MarketDataIncrementalRefresh&) = 0;
    virtual void operator()(const fix::MarketDataRequestReject&) = 0;
    virtual void operator()(const fix::MarketDataSnapshotFullRefresh&) = 0;
    virtual void operator()(const fix::OrderCancelReject&) = 0;
    virtual void operator()(const fix::PositionReport&) = 0;
    virtual void operator()(const fix::Reject&) = 0;
    virtual void operator()(const fix::SecurityList&) = 0;
    virtual void operator()(const fix::SecurityStatus&) = 0;
    virtual void operator()(const fix::UserResponse&) = 0;
  };
  FIX(
      Handler& handler,
      const Config& config,
      Random& random,
      core::event::Base& base,
      core::event::DNSBase& dns_base);

  FIX(const FIX&) = delete;
  FIX(FIX&&) = delete;

  bool ready() const;

  void close();

  void operator()(const server::StartEvent&);
  void operator()(const server::StopEvent&);
  void operator()(const server::TimerEvent&);

  void operator()(const fix::SecurityListRequest&);
  void operator()(const fix::SecurityStatusRequest&);
  void operator()(const fix::MarketDataRequest&);

  void operator()(const fix::UserRequest&);
  void operator()(const fix::RequestForPositions&);
  void operator()(const fix::OrderMassStatusRequest&);

  void operator()(const fix::NewOrderSingle&);
  void operator()(const fix::OrderCancelReplaceRequest&);
  void operator()(const fix::OrderCancelRequest&);

  void operator()(metrics::Writer& writer);

 protected:
  void operator()(const core::net::Manager::Connected&) override;
  void operator()(const core::net::Manager::Disconnected&) override;
  void operator()(const core::net::Manager::Read&) override;

 private:
  template <typename T>
  void send(const T& event);

  template <typename T>
  void send(
      const T& event,
      std::chrono::nanoseconds sending_time);

  void send_logon();
  void send_logout(const std::string_view& text);
  void send_heartbeat(const std::string_view& test_req_id);
  void send_test_request(std::chrono::nanoseconds now);

  void check(const core::fix::header_t& header);

  void parse(const core::fix::message_t& message);
  void parse_helper(const core::fix::message_t& message);

  void operator()(
      const core::fix::header_t&,
      const fix::Heartbeat&);
  void operator()(
      const core::fix::header_t&,
      const fix::Logon&);
  void operator()(
      const core::fix::header_t&,
      const fix::Logout&);
  void operator()(
      const core::fix::header_t&,
      const fix::ResendRequest&);
  void operator()(
      const core::fix::header_t&,
      const fix::TestRequest&);

  void operator()(
      const core::fix::header_t&,
      const fix::ExecutionReport&);
  void operator()(
      const core::fix::header_t&,
      const fix::MarketDataIncrementalRefresh&);
  void operator()(
      const core::fix::header_t&,
      const fix::MarketDataRequestReject&);
  void operator()(
      const core::fix::header_t&,
      const fix::MarketDataSnapshotFullRefresh&);
  void operator()(
      const core::fix::header_t&,
      const fix::OrderCancelReject&);
  void operator()(
      const core::fix::header_t&,
      const fix::PositionReport&);
  void operator()(
      const core::fix::header_t&,
      const fix::Reject&);
  void operator()(
      const core::fix::header_t&,
      const fix::SecurityList&);
  void operator()(
      const core::fix::header_t&,
      const fix::SecurityStatus&);
  void operator()(
      const core::fix::header_t&,
      const fix::UserResponse&);

 private:
  Handler& _handler;
  // config
  const std::string _access_key;
  // authentication
  Random& _random;
  // connection
  core::net::TcpConnectionFactory _connection_factory;
  core::net::Manager _connection;
  // buffers
  core::utils::Buffer _encode_buffer;
  core::utils::Buffer _decode_buffer;
  core::stack::Buffer<char, 32> _stack_buffer;
  // metrics
  struct {
    core::metrics::Counter
      disconnect;
  } _counter;
  struct {
    core::metrics::Profile
      parse,
      execution_report,
      market_data_incremental_refresh,
      market_data_request_reject,
      market_data_snapshot_full_refresh,
      order_cancel_reject,
      position_report,
      reject,
      security_list,
      security_status,
      user_response;
  } _profile;
  struct {
    core::metrics::Latency ping;
  } _latency;
  // state
  struct {
    uint64_t msg_seq_num = 0;
  } _outbound;
  struct {
    uint64_t msg_seq_num = 0;
  } _inbound;
  bool _ready = false;
  std::chrono::nanoseconds _next_heartbeat = {};
};

}  // namespace deribit
}  // namespace roq
