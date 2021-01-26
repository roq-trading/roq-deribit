/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <string>
#include <vector>

#include "roq/download.h"
#include "roq/server.h"

#include "roq/core/ssl/ssl.h"

#include "roq/core/event/base.h"
#include "roq/core/event/dns_base.h"

#include "roq/deribit/config.h"
#include "roq/deribit/fix.h"
#include "roq/deribit/random.h"
#include "roq/deribit/web_socket.h"

#include "roq/deribit/fix_state.h"
#include "roq/deribit/web_socket_state.h"

namespace roq {
namespace deribit {

class Gateway final : public server::Handler, public WebSocket::Handler, public FIX::Handler {
 public:
  Gateway(server::Dispatcher &dispatcher, const Config &config);

 protected:
  // server::Handler

  void operator()(const Event<Start> &) override;
  void operator()(const Event<Stop> &) override;
  void operator()(const Event<Timer> &) override;
  void operator()(const Event<Connection> &) override;

  void operator()(
      const Event<CreateOrder> &event,
      const std::string_view &request_id,
      uint32_t gateway_order_id) override;
  void operator()(
      const Event<ModifyOrder> &event,
      const std::string_view &request_id,
      const server::OMS_Order &order) override;
  void operator()(
      const Event<CancelOrder> &event,
      const std::string_view &request_id,
      const server::OMS_Order &order) override;

  void operator()(metrics::Writer &writer) override;

  // WebSocket::Handler

  void operator()(const WebSocket &) override;

  void operator()(const core::web::Socket::Latency &, const server::TraceInfo &) override;

  void operator()(const json::Currencies &, const server::TraceInfo &) override;
  void operator()(const json::Instruments &, const server::TraceInfo &) override;
  void operator()(const json::Positions &, const server::TraceInfo &) override;
  void operator()(const json::Ticker &, const server::TraceInfo &) override;

  // FIX::Handler

  void operator()(const FIX &) override;

  void operator()(
      const fix::Heartbeat &, const server::TraceInfo &, std::chrono::nanoseconds latency) override;

  void operator()(const fix::ExecutionReport &, const server::TraceInfo &) override;
  void operator()(const fix::MarketDataIncrementalRefresh &, const server::TraceInfo &) override;
  void operator()(const fix::MarketDataRequestReject &, const server::TraceInfo &) override;
  void operator()(const fix::MarketDataSnapshotFullRefresh &, const server::TraceInfo &) override;
  void operator()(const fix::OrderCancelReject &, const server::TraceInfo &) override;
  void operator()(const fix::PositionReport &, const server::TraceInfo &) override;
  void operator()(const fix::Reject &, const server::TraceInfo &) override;
  void operator()(const fix::SecurityList &, const server::TraceInfo &) override;
  void operator()(const fix::SecurityStatus &, const server::TraceInfo &) override;
  void operator()(const fix::UserResponse &, const server::TraceInfo &) override;

 private:
  using FIXDownload = server::Download<FIXState>;

  uint32_t download(FIXDownload::State state);

  using WebSocketDownload = server::Download<WebSocketState>;

  uint32_t download(WebSocketDownload::State state);

  void update(GatewayStatus gateway_status);

  void download_securities();
  void download_positions();
  void download_orders();
  void download_user();

  void subscribe_market_data();

 private:
  server::Dispatcher &dispatcher_;
  // config
  const std::string account_;
  const std::string access_key_;
  // authentication
  Random random_;
  // async
  core::event::Base base_;
  core::event::DNSBase dns_base_;
  // crypto
  core::ssl::Context ssl_context_;
  // fix
  struct {
    FIX connection;
    FIXDownload download;
  } fix_;
  // web socket
  struct {
    WebSocket connection;
    WebSocketDownload download;
  } web_socket_;
  // download (fix)
  absl::flat_hash_set<std::string> currencies_;
  std::vector<std::string> symbols_;
  // download (web socket)
  std::vector<std::string> currencies_2_;
  std::vector<std::string> symbols_2_;
  absl::flat_hash_map<std::string, roq::Layer> top_of_book_;
  absl::flat_hash_map<std::string, TradingStatus> trading_status_;
  // market data + order manager
  GatewayStatus gateway_status_ = GatewayStatus::DISCONNECTED;
  // order manager
  core::page_aligned_vector<Fill> fill_;
  // market data
  core::page_aligned_vector<MBPUpdate> bid_, ask_;
  core::page_aligned_vector<Trade> trade_;
  core::page_aligned_vector<Statistics> statistics_;
};

}  // namespace deribit
}  // namespace roq
