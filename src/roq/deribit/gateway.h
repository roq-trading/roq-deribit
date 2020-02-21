/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "roq/server.h"

#include "roq/core/hash/map.h"
#include "roq/core/hash/set.h"

#include "roq/core/event/base.h"
#include "roq/core/event/dns_base.h"

#include "roq/core/oms/order_cache.h"

#include "roq/deribit/config.h"
#include "roq/deribit/fix.h"
#include "roq/deribit/random.h"

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
  void update(GatewayStatus gateway_status);

  void begin_download();
  void check_download();

  void download_securities();
  void download_positions();
  void download_orders();
  void download_user();

  void subscribe_market_data();

  void reset();

  template <typename T>
  void enqueue(
      const T& value,
      bool is_last);

  template <typename T>
  void enqueue(
      uint8_t user_id,
      const T& value,
      bool is_last);

 private:
  server::Dispatcher& _dispatcher;
  // config
  const std::string _account;
  const std::string _access_key;
  // authentication
  Random _random;
  // async
  core::event::Base _base;
  core::event::DNSBase _dns_base;
  // connections
  FIX _fix;
  // download
  enum class Download {
    NONE,
    SECURITIES,
    POSITIONS,
    ORDERS,
    USER,
  } _download = Download::NONE;
  uint32_t _download_execution_reports = 0;
  uint32_t _download_users = 0;
  core::hash::set<std::string> _currencies;
  std::vector<std::string> _symbols;
  // market data + order manager
  GatewayStatus _gateway_status = GatewayStatus::DISCONNECTED;
  // market data
  core::page_aligned_vector<MBPUpdate> _bid, _ask;
  core::page_aligned_vector<Trade> _trade;
  // order manager
  core::oms::OrderCache _order_cache;
};

}  // namespace deribit
}  // namespace roq
