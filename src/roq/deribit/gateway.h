/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "roq/server.h"

#include "roq/core/uri.h"
#include "roq/core/clock.h"
#include "roq/core/utils/buffer.h"
#include "roq/core/ssl/ssl.h"
#include "roq/core/event/event.h"

#include "roq/deribit/config.h"
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
      const Config& config);

  void operator()(const StartEvent&) override;
  void operator()(const StopEvent&) override;
  void operator()(const TimerEvent&) override;
  void operator()(const ConnectionStatusEvent&) override;
  void operator()(const CreateOrderEvent&) override;
  void operator()(const ModifyOrderEvent&) override;
  void operator()(const CancelOrderEvent&) override;

  void write(Metrics& metrics) const override;

 protected:
  void run();
  void initialize_thread();
  void on_timer();

  void create_fix();

 public:
  void on_fix_connected();
  void on_fix_disconnected();
  void operator()(
      const core::fix::header_t& header,
      const fix::ExecutionReport& execution_report);
  void operator()(
      const core::fix::header_t& header,
      const fix::Heartbeat& heartbeat);
  void operator()(
      const core::fix::header_t& header,
      const fix::Logon& logon);
  void operator()(
      const core::fix::header_t& header,
      const fix::Logout& logout);
  void operator()(
      const core::fix::header_t& header,
      const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh);
  void operator()(
      const core::fix::header_t& header,
      const fix::MarketDataRequestReject& market_data_request_reject);
  void operator()(
      const core::fix::header_t& header,
      const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh);
  void operator()(
      const core::fix::header_t& header,
      const fix::OrderCancelReject& order_cancel_reject);
  void operator()(
      const core::fix::header_t& header,
      const fix::PositionReport& position_report);
  void operator()(
      const core::fix::header_t& header,
      const fix::Reject& reject);
  void operator()(
      const core::fix::header_t& header,
      const fix::ResendRequest& resend_request);
  void operator()(
      const core::fix::header_t& header,
      const fix::SecurityList& security_list);
  void operator()(
      const core::fix::header_t& header,
      const fix::TestRequest& test_request);
  void operator()(
      const core::fix::header_t& header,
      const fix::UserResponse& user_response);

 private:
  bool discard_symbol(const std::string_view& symbol);

  void update(GatewayStatus gateway_status);

  void begin_download();

  void check_download();

  void download_securities();
  void download_positions();
  void download_orders();
  void download_user();

  void subscribe_market_data();

  void reset();

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

 private:
  template <typename T>
  void send(const T& event) {
    send(event, core::get_realtime_clock());
  }
  template <typename T>
  void send(
      const T& event,
      const std::chrono::nanoseconds sending_time) {
    assert(static_cast<bool>(_fix));  // a check missing somehwere else
    if (static_cast<bool>(_fix) == false) return;  // FIXME(thraneh): DEBUG
    auto message = event.encode(
        _encode_buffer,
        _msg_seq_num,
        sending_time);
    // message.print();  // DEBUG
    _fix->send(message);
  }

  std::string get_next_request_id();

 private:
  server::Dispatcher& _dispatcher;
  core::ssl::Context _ssl_context;
  core::event::Base _base;
  core::event::DNSBase _dns_base;
  core::event::Timer _timer;
  std::atomic<bool> _stop = {false};
  std::thread _thread;
  core::utils::Buffer _encode_buffer;
  std::chrono::nanoseconds _next_update = {};
  // fix:
  std::unique_ptr<FIX> _fix;
  const std::string _access_key;
  const std::string _access_secret;
  uint64_t _msg_seq_num = 0;
  // gateway:
  std::chrono::nanoseconds _fix_reconnect_time = {};
  GatewayStatus _gateway_status = GatewayStatus::DISCONNECTED;
  enum class Download {
    NONE,
    SECURITIES,
    POSITIONS,
    ORDERS,
    USER,
  } _download = Download::NONE;
  uint32_t _download_execution_reports = 0;
  uint32_t _request_id = 0;
  // ...
  std::vector<std::regex> _symbols_regex;
  std::vector<MBPUpdate> _bid, _ask;
  std::vector<Trade> _trade;
  std::string _account;
  std::vector<std::string> _symbols;
  // ...
  Histogram<
    10000,  // 10us
    100000,
    1000000,  // 1ms
    10000000,
    100000000,
    1000000000  // 1s
    > _fix_latency;
};

}  // namespace deribit
}  // namespace roq
