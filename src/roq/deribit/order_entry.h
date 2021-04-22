/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

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
#include "roq/deribit/fix/reject.h"  // ... normally session level

// business (outbound)
#include "roq/deribit/fix/new_order_single.h"
#include "roq/deribit/fix/order_cancel_replace_request.h"
#include "roq/deribit/fix/order_cancel_request.h"
#include "roq/deribit/fix/order_mass_status_request.h"

namespace roq {
namespace deribit {

class OrderEntry final : public core::net::Manager::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<TradeUpdate> &, bool is_last, uint8_t user_id) = 0;
  };

  OrderEntry(Handler &, core::io::Context &, uint16_t stream_id, Security &, Shared &);

  OrderEntry(const OrderEntry &) = delete;
  OrderEntry(OrderEntry &&) = delete;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(
      const Event<CreateOrder> &, const std::string_view &request_id, uint32_t gateway_order_id);
  void operator()(
      const Event<ModifyOrder> &, const std::string_view &request_id, const server::OMS_Order &);
  void operator()(
      const Event<CancelOrder> &, const std::string_view &request_id, const server::OMS_Order &);
  void operator()(const Event<CancelAllOrders> &, const std::string_view &request_id);

  void operator()(metrics::Writer &);

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

  void download_orders();

  void parse(const core::fix::message_t &);
  void parse_helper(const core::fix::message_t &);

  void operator()(const core::fix::header_t &, const fix::Heartbeat &, const server::TraceInfo &);
  void operator()(const core::fix::header_t &, const fix::Logon &, const server::TraceInfo &);
  void operator()(const core::fix::header_t &, const fix::Logout &, const server::TraceInfo &);
  void operator()(
      const core::fix::header_t &, const fix::ResendRequest &, const server::TraceInfo &);
  void operator()(const core::fix::header_t &, const fix::TestRequest &, const server::TraceInfo &);

  void operator()(
      const core::fix::header_t &, const fix::ExecutionReport &, const server::TraceInfo &);
  void operator()(
      const core::fix::header_t &, const fix::OrderCancelReject &, const server::TraceInfo &);
  void operator()(const core::fix::header_t &, const fix::Reject &, const server::TraceInfo &);

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
  // connection
  core::net::TcpConnectionFactory connection_factory_;
  core::net::Manager connection_;
  // buffers
  core::Buffer encode_buffer_;
  core::Buffer decode_buffer_;
  core::stack::Buffer<char, 32u> stack_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, execution_report, order_cancel_reject, reject;
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
};

}  // namespace deribit
}  // namespace roq
