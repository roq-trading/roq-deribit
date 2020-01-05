/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "roq/server.h"

#include "roq/core/uri.h"
#include "roq/core/clock.h"
#include "roq/core/hash_map.h"
#include "roq/core/hash_set.h"
#include "roq/core/utils/buffer.h"
#include "roq/core/ssl/ssl.h"

#include "roq/core/event/base.h"
#include "roq/core/event/dns_base.h"

#include "roq/deribit/config.h"
#include "roq/deribit/fix.h"
#include "roq/deribit/order_mapping.h"

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

  void operator()(Metrics& metrics) override;

 protected:
  void create_fix();

 public:
  void on_fix_connected();
  void on_fix_disconnected();
  void operator()(
      const core::fix::header_t&,
      const fix::ExecutionReport&);
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
      const fix::ResendRequest&);
  void operator()(
      const core::fix::header_t&,
      const fix::SecurityList&);
  void operator()(
      const core::fix::header_t&,
      const fix::TestRequest&);
  void operator()(
      const core::fix::header_t&,
      const fix::UserResponse&);

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
  template <typename T>
  void enqueue(
      uint8_t user_id,
      const T& event,
      bool is_last) {
    auto now = core::get_system_clock();
    _dispatcher.enqueue(
        user_id,
        event,
        now,
        now,
        is_last);
  }

 private:
  template <typename T>
  void send(const T& event);
  template <typename T>
  void send(
      const T& event,
      const std::chrono::nanoseconds sending_time);

  std::string create_request_id();

  void check(const core::fix::header_t& header);

  inline auto create_order_id() {
    return ++_local_order_id;
  }
  inline auto create_trade_id() {
    return ++_local_trade_id;
  }

 private:
  server::Dispatcher& _dispatcher;
  core::ssl::Context _ssl_context;
  core::event::Base _base;
  core::event::DNSBase _dns_base;
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
  uint32_t _download_users = 0;
  // ...
  std::vector<std::regex> _symbols_regex;
  std::vector<MBPUpdate> _bid, _ask;
  std::vector<Trade> _trade;
  std::string _account;
  std::vector<std::string> _symbols;
  // ...
  ExternalLatency _fix_latency;

 private:
  InternalLatency _market_data_incremental_refresh;

  uint64_t _their_msg_seq_num = 0;

  uint32_t _local_order_id = 10000000;  // TODO(thraneh): we need this extracted from the feed

  std::unordered_map<uint64_t, OrderMapping> _order_mapping;

  core::hash_map<std::string, uint64_t> _order_lookup;

  uint32_t _local_trade_id = 20000000;

  decltype(_order_mapping)::iterator find_order_mapping(
      const std::string_view& cl_ord_id,
      const std::string_view& orig_cl_ord_id);

  decltype(_order_mapping)::iterator create_order_mapping(
      const fix::ExecutionReport& execution_report);

  core::hash_set<std::string> _currencies;
};

}  // namespace deribit
}  // namespace roq
