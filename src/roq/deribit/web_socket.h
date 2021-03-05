/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/web/socket.h"

#include "roq/core/jsonrpc/parser.h"

#include "roq/download.h"
#include "roq/server.h"

#include "roq/deribit/security.h"
#include "roq/deribit/shared.h"
#include "roq/deribit/web_socket_state.h"

#include "roq/deribit/json/auth.h"
#include "roq/deribit/json/currencies.h"
#include "roq/deribit/json/instruments.h"
#include "roq/deribit/json/parser.h"
#include "roq/deribit/json/positions.h"
#include "roq/deribit/json/ticker.h"

namespace roq {
namespace deribit {

class WebSocket final : public core::web::Socket::Handler,
                        public core::jsonrpc::Parser::Handler,
                        public json::Parser::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;

    virtual void operator()(const server::Trace<MarketDataStatus> &) = 0;

    virtual void operator()(const server::Trace<TopOfBook> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<MarketStatus> &, bool is_last) = 0;
  };

  WebSocket(Handler &, core::io::Context &, uint16_t stream_id, Security &, Shared &);

  WebSocket(WebSocket &&) = delete;
  WebSocket(const WebSocket &) = delete;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

 protected:
  void operator()(const core::web::Socket::Connected &) override;
  void operator()(const core::web::Socket::Disconnected &) override;
  void operator()(const core::web::Socket::Ready &) override;
  void operator()(const core::web::Socket::Close &) override;
  void operator()(const core::web::Socket::Latency &) override;
  void operator()(const core::web::Socket::Text &) override;

 private:
  void operator()(GatewayStatus);

  void login();

  uint32_t download(WebSocketState);

  uint32_t download_currencies();
  uint32_t download_instruments();
  uint32_t download_positions();
  uint32_t download_tickers();

  void get_currencies();
  void get_instruments(const std::string_view &currency);
  void get_positions(const std::string_view &currency);

  void subscribe_ticker(const roq::span<std::string> &symbols);

  void unsubscribe_ticker(const roq::span<std::string> &symbols);

  void parse(const std::string_view &message);

  void operator()(const core::jsonrpc::Error &, core::json::value_t &) override;
  void operator()(const core::jsonrpc::Result &, core::json::value_t &) override;
  void operator()(const core::jsonrpc::Notification &, core::json::value_t &) override;

  void operator()(const json::Auth &, const server::TraceInfo &);

  void operator()(const json::Currencies &, const server::TraceInfo &);
  void operator()(const json::Instruments &, const server::TraceInfo &);
  void operator()(const json::Positions &, const server::TraceInfo &);

  void operator()(const json::Ticker &, const server::TraceInfo &) override;

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  // web socket
  core::web::Socket connection_;
  // buffers
  core::utils::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, auth, currencies, instruments, positions, ticker;
  } profile_;
  struct {
    core::metrics::Latency ping, heartbeat;
  } latency_;
  // security
  Security &security_;
  // cache
  Shared &shared_;
  std::vector<std::string> currencies_;
  std::vector<std::string> symbols_;
  absl::flat_hash_map<std::string, roq::Layer> top_of_book_;
  absl::flat_hash_map<std::string, TradingStatus> trading_status_;
  // state
  bool ready_ = false;
  GatewayStatus status_ = {};
  server::Download<WebSocketState> download_;
};

}  // namespace deribit
}  // namespace roq
