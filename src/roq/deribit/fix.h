/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string>

#include "roq/core/stack/buffer.h"

#include "roq/core/metrics/profile.h"
#include "roq/core/metrics/latency.h"

#include "roq/core/event/base.h"
#include "roq/core/event/dns_base.h"

#include "roq/core/net/manager.h"
#include "roq/core/net/tcp_connection_factory.h"

#include "roq/deribit/config.h"

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
#include "roq/deribit/fix/user_response.h"

// business (outbound)
#include "roq/deribit/fix/market_data_request.h"
#include "roq/deribit/fix/new_order_single.h"
#include "roq/deribit/fix/order_cancel_replace_request.h"
#include "roq/deribit/fix/order_cancel_request.h"
#include "roq/deribit/fix/order_mass_status_request.h"
#include "roq/deribit/fix/request_for_positions.h"
#include "roq/deribit/fix/security_list_request.h"
#include "roq/deribit/fix/user_request.h"

namespace roq {
namespace deribit {

class Gateway;

class FIX final : public core::net::Manager::Handler {
  enum class State {
    DISCONNECTED,
    LOGON_SENT,
    READY,
  };

 public:
  FIX(
      Gateway& gateway,
      const Config& config,
      core::event::Base& base,
      core::event::DNSBase& dns_base);

  FIX(const FIX&) = delete;
  FIX(FIX&&) = delete;

  void operator=(const FIX&) = delete;
  void operator=(FIX&&) = delete;

  bool ready() const;

  std::string_view next_request_id();

  void operator()(const StartEvent&);
  void operator()(const StopEvent&);
  void operator()(const TimerEvent&);

  void operator()(const fix::MarketDataRequest&);
  void operator()(const fix::NewOrderSingle&);
  void operator()(const fix::OrderCancelReplaceRequest&);
  void operator()(const fix::OrderCancelRequest&);
  void operator()(const fix::OrderMassStatusRequest&);
  void operator()(const fix::RequestForPositions&);
  void operator()(const fix::SecurityListRequest&);
  void operator()(const fix::UserRequest&);

  void operator()(Metrics& metrics);

 protected:
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

  void operator()(State state);

  void operator()(const core::net::Manager::Connected&) override;
  void operator()(const core::net::Manager::Disconnected&) override;
  void operator()(const core::net::Manager::Read&) override;

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
      const fix::UserResponse&);

 private:
  Gateway& _gateway;
  // config
  const std::string _access_key;
  const std::string _access_secret;
  // connection
  core::net::TcpConnectionFactory _connection_factory;
  core::net::Manager _connection;
  // buffers
  core::utils::Buffer _encode_buffer;
  core::utils::Buffer _decode_buffer;
  // session
  State _state = State::DISCONNECTED;
  uint64_t _msg_seq_num = 0;
  std::chrono::nanoseconds _next_heartbeat = {};
  uint64_t _their_msg_seq_num = 0;
  // other
  core::stack::Buffer<char, 32> _buffer;
  uint32_t _request_id = 0;
  // metrics
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
      user_response;
  } _profile;
  struct {
    core::metrics::Latency ping;
  } _latency;
};

}  // namespace deribit
}  // namespace roq
