/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "roq/server.h"
#include "roq/download.h"

#include "roq/core/hash/map.h"
#include "roq/core/hash/set.h"

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

class Gateway final
    : public server::Handler,
      public WebSocket::Handler,
      public FIX::Handler {
 public:
  Gateway(
      server::Dispatcher& dispatcher,
      const Config& config);

 protected:
  // server::Handler

  void operator()(const server::StartEvent&) override;
  void operator()(const server::StopEvent&) override;
  void operator()(const server::TimerEvent&) override;
  void operator()(const server::ConnectionStatusEvent&) override;

  void operator()(
      const CreateOrderEvent& event,
      const std::string_view& request_id,
      uint32_t gateway_order_id) override;
  void operator()(
      const ModifyOrderEvent& event,
      const std::string_view& request_id,
      const server::OMS_Order& order) override;
  void operator()(
      const CancelOrderEvent& event,
      const std::string_view& request_id,
      const server::OMS_Order& order) override;

  void operator()(metrics::Writer& writer) override;

  // WebSocket::Handler

  void operator()(const WebSocket&) override;
  void operator()(const json::Currencies&) override;
  void operator()(const json::Instruments&) override;
  void operator()(const json::Positions&) override;
  void operator()(const json::Ticker&) override;

  // FIX::Handler

  void operator()(const FIX&) override;
  void operator()(const fix::ExecutionReport&) override;
  void operator()(const fix::MarketDataIncrementalRefresh&) override;
  void operator()(const fix::MarketDataRequestReject&) override;
  void operator()(const fix::MarketDataSnapshotFullRefresh&) override;
  void operator()(const fix::OrderCancelReject&) override;
  void operator()(const fix::PositionReport&) override;
  void operator()(const fix::Reject&) override;
  void operator()(const fix::SecurityList&) override;
  void operator()(const fix::SecurityStatus&) override;
  void operator()(const fix::UserResponse&) override;

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
  // crypto
  core::ssl::Context _ssl_context;
  // fix
  struct {
    FIX connection;
    FIXDownload download;
  } _fix;
  // web socket
  struct  {
    WebSocket connection;
    WebSocketDownload download;
  } _web_socket;
  // download (fix)
  core::hash::set<std::string> _currencies;
  std::vector<std::string> _symbols;
  core::hash::map<std::string, TradingStatus> _trading_status;
  // download (web socket)
  std::vector<std::string> _currencies_2;
  std::vector<std::string> _symbols_2;
  // market data + order manager
  GatewayStatus _gateway_status = GatewayStatus::DISCONNECTED;
  // order manager
  core::page_aligned_vector<Fill> _fill;
  // market data
  core::page_aligned_vector<MBPUpdate> _bid, _ask;
  core::page_aligned_vector<Trade> _trade;
};

}  // namespace deribit
}  // namespace roq
