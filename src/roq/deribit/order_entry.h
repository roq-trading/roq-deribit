/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <string>

#include "roq/core/stack/buffer.h"

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/net/manager.h"
#include "roq/core/net/tcp_connection_factory.h"

#include "roq/download.h"
#include "roq/server.h"

#include "roq/deribit/order_entry_state.h"
#include "roq/deribit/security.h"
#include "roq/deribit/shared.h"

// session
#include "roq/deribit/fix/heartbeat.h"
#include "roq/deribit/fix/logon.h"
#include "roq/deribit/fix/logout.h"
#include "roq/deribit/fix/resend_request.h"
#include "roq/deribit/fix/test_request.h"

// business (inbound)
#include "roq/deribit/fix/execution_report.h"
#include "roq/deribit/fix/order_cancel_reject.h"
#include "roq/deribit/fix/order_mass_cancel_report.h"
#include "roq/deribit/fix/position_report.h"
#include "roq/deribit/fix/reject.h"  // ... normally session level

namespace roq {
namespace deribit {

class OrderEntry final : public core::net::Manager::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<TradeUpdate> &, bool is_last, uint8_t user_id) = 0;
    virtual void operator()(const server::Trace<PositionUpdate> &, bool is_last) = 0;
  };

  OrderEntry(Handler &, core::io::Context &, uint16_t stream_id, Security &, Shared &);

  OrderEntry(const OrderEntry &) = delete;
  OrderEntry(OrderEntry &&) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  uint16_t operator()(const Event<CreateOrder> &, const std::string_view &request_id);
  uint16_t operator()(
      const Event<ModifyOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id);
  uint16_t operator()(
      const Event<CancelOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id);

  uint16_t operator()(const Event<CancelAllOrders> &, const std::string_view &request_id);

  void operator()(metrics::Writer &);

  void operator()(const core::fix::Event<fix::Heartbeat> &, const server::TraceInfo &);
  void operator()(const core::fix::Event<fix::Logon> &, const server::TraceInfo &);
  void operator()(const core::fix::Event<fix::Logout> &, const server::TraceInfo &);
  void operator()(const core::fix::Event<fix::ResendRequest> &, const server::TraceInfo &);
  void operator()(const core::fix::Event<fix::TestRequest> &, const server::TraceInfo &);

  void operator()(const core::fix::Event<fix::PositionReport> &, const server::TraceInfo &);

  void operator()(const core::fix::Event<fix::ExecutionReport> &, const server::TraceInfo &);
  void operator()(const core::fix::Event<fix::OrderCancelReject> &, const server::TraceInfo &);
  void operator()(const core::fix::Event<fix::Reject> &, const server::TraceInfo &);
  void operator()(const core::fix::Event<fix::OrderMassCancelReport> &, const server::TraceInfo &);

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

  uint32_t download(OrderEntryState);

  void subscribe_positions();
  void download_orders();

  void parse(const core::fix::message_t &);
  void parse_helper(const core::fix::message_t &);

  // utilities

  template <typename T>
  uint64_t send(const T &event);

  template <typename T>
  uint64_t send(const T &event, std::chrono::nanoseconds sending_time);

  void check(const core::fix::header_t &);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
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
    core::metrics::Profile parse, position_report, execution_report, order_cancel_reject, reject,
        order_mass_cancel_report;
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
  absl::flat_hash_set<std::string> all_currencies_;  // only master
  // state
  bool ready_ = false;
  std::chrono::nanoseconds next_heartbeat_ = {};
  ConnectionStatus status_ = {};
  server::Download<OrderEntryState> download_;
  std::chrono::nanoseconds last_logon_or_heartbeat_ = {};
  // EXPERIMENTAL
  absl::flat_hash_map<uint64_t, std::string> msg_seq_num_to_request_id_;
};

}  // namespace deribit
}  // namespace roq
