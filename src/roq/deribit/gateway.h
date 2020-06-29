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

  void operator()(const Event<Start>&) override;
  void operator()(const Event<Stop>&) override;
  void operator()(const Event<Timer>&) override;
  void operator()(const Event<Connection>&) override;

  void operator()(
      const Event<CreateOrder>& event,
      const std::string_view& request_id,
      uint32_t gateway_order_id) override;
  void operator()(
      const Event<ModifyOrder>& event,
      const std::string_view& request_id,
      const server::OMS_Order& order) override;
  void operator()(
      const Event<CancelOrder>& event,
      const std::string_view& request_id,
      const server::OMS_Order& order) override;

  void operator()(metrics::Writer& writer) override;

  // WebSocket::Handler

  void operator()(const WebSocket&) override;

  void operator()(
      const json::Currencies&,
      const server::Trace&) override;
  void operator()(
      const json::Instruments&,
      const server::Trace&) override;
  void operator()(
      const json::Positions&,
      const server::Trace&) override;
  void operator()(
      const json::Ticker&,
      const server::Trace&) override;

  // FIX::Handler

  void operator()(const FIX&) override;

  void operator()(
      const fix::ExecutionReport&,
      const server::Trace&) override;
  void operator()(
      const fix::MarketDataIncrementalRefresh&,
      const server::Trace&) override;
  void operator()(
      const fix::MarketDataRequestReject&,
      const server::Trace&) override;
  void operator()(
      const fix::MarketDataSnapshotFullRefresh&,
      const server::Trace&) override;
  void operator()(
      const fix::OrderCancelReject&,
      const server::Trace&) override;
  void operator()(
      const fix::PositionReport&,
      const server::Trace&) override;
  void operator()(
      const fix::Reject&,
      const server::Trace&) override;
  void operator()(
      const fix::SecurityList&,
      const server::Trace&) override;
  void operator()(
      const fix::SecurityStatus&,
      const server::Trace&) override;
  void operator()(
      const fix::UserResponse&,
      const server::Trace&) override;

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
      const server::Trace& trace,
      bool is_last);

  template <typename T>
  void enqueue(
      uint8_t user_id,
      const T& value,
      const server::Trace& trace,
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
  core::page_aligned_vector<Statistics> _statistics;
};

}  // namespace deribit
}  // namespace roq
