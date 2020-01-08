/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "roq/server.h"

#include "roq/core/hash_map.h"
#include "roq/core/hash_set.h"

#include "roq/core/event/base.h"
#include "roq/core/event/dns_base.h"

#include "roq/deribit/config.h"
#include "roq/deribit/fix.h"
#include "roq/deribit/order_mapping.h"

// fix (inbound)
#include "roq/deribit/fix/execution_report.h"
#include "roq/deribit/fix/market_data_incremental_refresh.h"
#include "roq/deribit/fix/market_data_request_reject.h"
#include "roq/deribit/fix/market_data_snapshot_full_refresh.h"
#include "roq/deribit/fix/order_cancel_reject.h"
#include "roq/deribit/fix/position_report.h"
#include "roq/deribit/fix/reject.h"
#include "roq/deribit/fix/security_list.h"
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

  // fix
  void operator()(const FIX&);
  void operator()(const fix::ExecutionReport&);
  void operator()(const fix::MarketDataIncrementalRefresh&);
  void operator()(const fix::MarketDataRequestReject&);
  void operator()(const fix::MarketDataSnapshotFullRefresh&);
  void operator()(const fix::OrderCancelReject&);
  void operator()(const fix::PositionReport&);
  void operator()(const fix::Reject&);
  void operator()(const fix::SecurityList&);
  void operator()(const fix::UserResponse&);

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
  void subscribe_market_data_batch();
  void subscribe_market_data_simple();

  void reset();

  template <typename T>
  void enqueue(
      const T& event,
      bool is_last);

  template <typename T>
  void enqueue(
      const T& event,
      bool is_last,
      std::chrono::nanoseconds origin_create_time);

  template <typename T>
  void enqueue(
      const T& event,
      bool is_last,
      const std::chrono::nanoseconds origin_create_time,
      const std::chrono::nanoseconds source_receive_time);

  template <typename T>
  void enqueue(
      uint8_t user_id,
      const T& event,
      bool is_last);

 private:
  inline auto create_order_id() {  // XXX review
    return ++_local_order_id;
  }
  inline auto create_trade_id() {  // XXX review
    return ++_local_trade_id;
  }

 private:
  server::Dispatcher& _dispatcher;
  // config
  const std::string _account;
  const std::string _access_key;
  const std::vector<std::regex> _symbols_regex;
  // async
  core::event::Base _base;
  core::event::DNSBase _dns_base;
  // connections
  FIX _fix;
  // gateway
  GatewayStatus _gateway_status = GatewayStatus::DISCONNECTED;
  // download
  enum class Download {
    NONE,
    SECURITIES,
    POSITIONS,
    ORDERS,
    USER,
  } _download = Download::NONE;
  std::vector<std::string> _symbols;
  uint32_t _download_execution_reports = 0;
  uint32_t _download_users = 0;


  // ...
  std::vector<MBPUpdate> _bid, _ask;
  std::vector<Trade> _trade;

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
