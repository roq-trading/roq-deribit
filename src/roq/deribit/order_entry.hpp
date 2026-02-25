/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>

#include "roq/utils/container.hpp"

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"
#include "roq/io/net/connection_factory.hpp"
#include "roq/io/net/connection_manager.hpp"

#include "roq/core/download.hpp"

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
    virtual void operator()(Trace<TradeUpdate> const &, bool is_last, uint8_t user_id, std::string_view const &request_id) = 0;
    virtual void operator()(Trace<PositionUpdate> const &, bool is_last) = 0;
  };

  OrderEntry(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  OrderEntry(OrderEntry const &) = delete;

  bool ready() const { return connection_status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  uint16_t operator()(Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id);
  uint16_t operator()(
      Event<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
  uint16_t operator()(
      Event<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id);

  void operator()(metrics::Writer &) const;

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
  void operator()(io::net::ConnectionManager::Write const &) override;

 private:
  void operator()(ConnectionStatus, std::string_view const &reason = {});

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
  uint64_t send(T const &event);

  template <typename T>
  uint64_t send(T const &event, std::chrono::nanoseconds sending_time);

  void check(roq::fix::Header const &);

  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // connection
  std::unique_ptr<io::net::ConnectionFactory> const connection_factory_;
  std::unique_ptr<io::net::ConnectionManager> const connection_manager_;
  // buffers
  std::vector<std::byte> decode_buffer_;
  std::vector<std::byte> encode_buffer_2_;
  std::string encode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse, position_report, execution_report, order_cancel_reject, reject, order_mass_cancel_report;
  } profile_;
  struct {
    utils::metrics::Latency ping;
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
  ConnectionStatus connection_status_ = {};

  // state
  bool ready_ = false;
  std::chrono::nanoseconds next_heartbeat_ = {};
  core::Download<OrderEntryState> download_;
  std::chrono::nanoseconds last_logon_or_heartbeat_ = {};
  // EXPERIMENTAL
  utils::unordered_map<uint64_t, RequestId> msg_seq_num_to_request_id_;
  std::chrono::nanoseconds test_disconnect_time_ = {};
  std::chrono::nanoseconds test_logon_time_ = {};
};

}  // namespace deribit
}  // namespace roq
