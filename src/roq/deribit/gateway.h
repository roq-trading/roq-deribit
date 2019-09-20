/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <string>
#include <thread>
#include <vector>

#include "roq/server.h"

#include "roq/core/uri.h"
#include "roq/core/clock.h"
#include "roq/core/utils/buffer.h"
#include "roq/core/ssl/ssl.h"
#include "roq/core/event/event.h"

#include "roq/deribit/conf/config.h"
#include "roq/deribit/fix.h"

#include "roq/deribit/fix/execution_report.h"
#include "roq/deribit/fix/heartbeat.h"
#include "roq/deribit/fix/logon.h"
#include "roq/deribit/fix/logout.h"
#include "roq/deribit/fix/market_data_incremental_refresh.h"
#include "roq/deribit/fix/market_data_request_reject.h"
#include "roq/deribit/fix/market_data_snapshot_full_refresh.h"
#include "roq/deribit/fix/order_cancel_reject.h"
#include "roq/deribit/fix/position_report.h"
#include "roq/deribit/fix/reject.h"
#include "roq/deribit/fix/resend_request.h"
#include "roq/deribit/fix/security_list.h"
#include "roq/deribit/fix/test_request.h"
#include "roq/deribit/fix/user_response.h"

namespace roq {
namespace deribit {

class Gateway final : public server::Handler {
 public:
  Gateway(
      server::Dispatcher& dispatcher,
      const conf::Config& config,
      const core::URI& ws_uri,
      const core::URI& fix_uri);

  void on(const StartEvent& event) override;
  void on(const StopEvent& event) override;
  void on(const TimerEvent& event) override;
  void on(const ConnectionStatusEvent& event) override;
  void on(const CreateOrderEvent& event) override;
  void on(const ModifyOrderEvent& event) override;
  void on(const CancelOrderEvent& event) override;

  void write(Metrics& metrics) const override;

 protected:
  void run();
  void initialize_thread();
  void on_timer();

 public:
  void on_fix_connected();
  void on_fix_disconnected();
  void operator()(const fix::ExecutionReport& execution_report, uint64_t seq_num);
  void operator()(const fix::Heartbeat& heartbeat, uint64_t seq_num);
  void operator()(const fix::Logon& logon, uint64_t seq_num);
  void operator()(const fix::Logout& logout, uint64_t seq_num);
  void operator()(const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh, uint64_t seq_num);
  void operator()(const fix::MarketDataRequestReject& market_data_request_reject, uint64_t seq_num);
  void operator()(const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh, uint64_t seq_num);
  void operator()(const fix::OrderCancelReject& order_cancel_reject, uint64_t seq_num);
  void operator()(const fix::PositionReport& position_report, uint64_t seq_num);
  void operator()(const fix::Reject& reject, uint64_t seq_num);
  void operator()(const fix::ResendRequest& resend_request, uint64_t seq_num);
  void operator()(const fix::SecurityList& security_list, uint64_t seq_num);
  void operator()(const fix::TestRequest& test_request, uint64_t seq_num);
  void operator()(const fix::UserResponse& user_response, uint64_t seq_num);

 private:
  template <typename T>
  void enqueue(
      const T& event,
      bool is_last) {
    auto now = core::get_system_clock();
    enqueue(event, is_last, now, now);
  }
  template <typename T>
  void enqueue(
      const T& event,
      bool is_last,
      std::chrono::nanoseconds origin_create_time) {
    auto now = core::get_system_clock();
    enqueue(event, is_last, origin_create_time, now);
  }
  template <typename T>
  void enqueue(
      const T& event,
      bool is_last,
      const std::chrono::nanoseconds origin_create_time,
      const std::chrono::nanoseconds source_receive_time) {
    _dispatcher.enqueue(
        event,
        source_receive_time,
        origin_create_time,
        is_last);
  }

  template <typename T>
  void send(const T& event) {
    send(event, core::get_realtime_clock());
  }
  template <typename T>
  void send(
      const T& event,
      const std::chrono::nanoseconds sending_time) {
    auto message = event.encode(
        _encode_buffer,
        _msg_seq_num,
        sending_time);
    _fix.send(message);
  }

 private:
  server::Dispatcher& _dispatcher;
  core::ssl::Context _ssl_context;
  core::event::Base _base;
  core::event::DNSBase _dns_base;
  core::event::Timer _timer;
  std::atomic<bool> _stop = {false};
  std::thread _thread;
  // ...
  core::ssl::Connection _ssl_connection;
  core::event::BufferEvent _buffer_event;
  // ...
  FIX _fix;
  // ...
  std::vector<std::byte> _decode_buffer;
  core::utils::Buffer _encode_buffer;
  std::chrono::nanoseconds _next_update = {};
  // fix:
  const std::string _access_key;
  const std::string _access_secret;
  uint64_t _msg_seq_num = 0;
};

}  // namespace deribit
}  // namespace roq
