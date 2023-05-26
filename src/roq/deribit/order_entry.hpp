/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <string>

#include "roq/core/download.hpp"

#include "roq/core/stack/buffer.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/io/context.hpp"
#include "roq/io/net/connection_factory.hpp"
#include "roq/io/net/connection_manager.hpp"

#include "roq/server.hpp"

#include "roq/deribit/account.hpp"
#include "roq/deribit/order_entry_state.hpp"
#include "roq/deribit/shared.hpp"

// session
#include "roq/deribit/fix/heartbeat.hpp"
#include "roq/deribit/fix/logon.hpp"
#include "roq/deribit/fix/logout.hpp"
#include "roq/deribit/fix/resend_request.hpp"
#include "roq/deribit/fix/test_request.hpp"

// business (inbound)
#include "roq/deribit/fix/execution_report.hpp"
#include "roq/deribit/fix/order_cancel_reject.hpp"
#include "roq/deribit/fix/order_mass_cancel_report.hpp"
#include "roq/deribit/fix/position_report.hpp"
#include "roq/deribit/fix/reject.hpp"  // ... normally session level

namespace roq {
namespace deribit {

struct OrderEntry final : public io::net::ConnectionManager::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(
        Trace<TradeUpdate> const &, bool is_last, uint8_t user_id, std::string_view const &request_id) = 0;
    virtual void operator()(Trace<PositionUpdate> const &, bool is_last) = 0;
  };

  OrderEntry(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  OrderEntry(OrderEntry const &) = delete;
  OrderEntry(OrderEntry &&) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  uint16_t operator()(Event<CreateOrder> const &, oms::Order const &, std::string_view const &request_id);
  uint16_t operator()(
      Event<ModifyOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
  uint16_t operator()(
      Event<CancelOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id);

  void operator()(metrics::Writer &);

  void operator()(Trace<fix::Heartbeat> const &, roq::fix::Header const &);
  void operator()(Trace<fix::Logon> const &, roq::fix::Header const &);
  void operator()(Trace<fix::Logout> const &, roq::fix::Header const &);
  void operator()(Trace<fix::ResendRequest> const &, roq::fix::Header const &);
  void operator()(Trace<fix::TestRequest> const &, roq::fix::Header const &);

  void operator()(Trace<fix::PositionReport> const &, roq::fix::Header const &);

  void operator()(Trace<fix::ExecutionReport> const &, roq::fix::Header const &);
  void operator()(Trace<fix::OrderCancelReject> const &, roq::fix::Header const &);
  void operator()(Trace<fix::Reject> const &, roq::fix::Header const &);
  void operator()(Trace<fix::OrderMassCancelReport> const &, roq::fix::Header const &);

 protected:
  void operator()(io::net::ConnectionManager::Connected const &) override;
  void operator()(io::net::ConnectionManager::Disconnected const &) override;
  void operator()(io::net::ConnectionManager::Read const &) override;

 private:
  void operator()(ConnectionStatus);

  void send_logon();
  void send_logout(std::string_view const &text);
  void send_heartbeat(std::string_view const &test_req_id);
  void send_test_request(std::chrono::nanoseconds now);

  uint32_t download(OrderEntryState);

  void subscribe_positions();
  void download_orders();

  void parse(Trace<roq::fix::Message> const &);
  void parse_helper(Trace<roq::fix::Message> const &);

  // utilities

  template <typename T>
  std::tuple<uint64_t, std::chrono::nanoseconds, std::chrono::nanoseconds> send(T const &event);

  template <typename T>
  std::tuple<uint64_t, std::chrono::nanoseconds, std::chrono::nanoseconds> send(
      T const &event, std::chrono::nanoseconds sending_time);

  void check(roq::fix::Header const &);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  Source const name_;
  // connection
  std::unique_ptr<io::net::ConnectionFactory> const connection_factory_;
  std::unique_ptr<io::net::ConnectionManager> const connection_manager_;
  // buffers
  std::vector<std::byte> decode_buffer_;
  std::string encode_buffer_;
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
  // account
  Account &account_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};

  // state
  bool ready_ = false;
  std::chrono::nanoseconds next_heartbeat_ = {};
  core::Download<OrderEntryState> download_;
  std::chrono::nanoseconds last_logon_or_heartbeat_ = {};
  // EXPERIMENTAL
  absl::flat_hash_map<uint64_t, RequestId> msg_seq_num_to_request_id_;
  std::chrono::nanoseconds test_disconnect_time_ = {};
  std::chrono::nanoseconds test_logon_time_ = {};
  bool const enable_round_trip_latency_;
};

}  // namespace deribit
}  // namespace roq
