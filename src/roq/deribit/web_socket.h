/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
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
  struct CurrenciesUpdate final {
    std::vector<std::string> &currencies;
  };
  struct SymbolsUpdate final {
    std::vector<std::string> &symbols;
  };

  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<TopOfBook> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<MarketStatus> &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(CurrenciesUpdate &) = 0;
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  WebSocket(Handler &, core::io::Context &, uint16_t stream_id, Shared &, bool master);

  WebSocket(WebSocket &&) = delete;
  WebSocket(const WebSocket &) = delete;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  void update_subscriptions(std::vector<std::string> &symbols);

 protected:
  void operator()(const core::web::Socket::Connected &) override;
  void operator()(const core::web::Socket::Disconnected &) override;
  void operator()(const core::web::Socket::Ready &) override;
  void operator()(const core::web::Socket::Close &) override;
  void operator()(const core::web::Socket::Latency &) override;
  void operator()(const core::web::Socket::Text &) override;

 private:
  void operator()(ConnectionStatus);

  uint32_t download(WebSocketState);

  uint32_t download_currencies();
  uint32_t download_instruments();

  void get_currencies();
  void get_instruments(const std::string_view &currency);

  void subscribe_platform_state();
  void subscribe_instrument_state();

  void subscribe_quote(const roq::span<std::string> &symbols);
  void subscribe_ticker(const roq::span<std::string> &symbols);

  void parse(const std::string_view &message);

  void operator()(const core::jsonrpc::Error &, core::json::value_t &) override;
  void operator()(const core::jsonrpc::Result &, core::json::value_t &) override;
  void operator()(const core::jsonrpc::Notification &, core::json::value_t &) override;

  void operator()(const json::Auth &, const server::TraceInfo &);

  void operator()(const json::Currencies &, const server::TraceInfo &);
  void operator()(const json::Instruments &, const server::TraceInfo &);
  void operator()(const json::Positions &, const server::TraceInfo &);

  void operator()(const server::Trace<json::PlatformState> &) override;
  void operator()(const server::Trace<json::InstrumentState> &) override;
  void operator()(const server::Trace<json::Quote> &) override;
  void operator()(const server::Trace<json::Ticker> &) override;
  void operator()(const server::Trace<json::Portfolio> &) override;
  void operator()(const server::Trace<json::Changes> &) override;
  void operator()(const server::Trace<json::Order> &) override;

  template <typename C>
  bool get_top_of_book(const std::string_view &symbol, C callback) {
    auto iter = top_of_book_.find(symbol);
    if (iter == top_of_book_.end()) {
      auto iter_2 = all_symbols_.find(symbol);
      if (iter_2 == all_symbols_.end())
        return false;
      iter = top_of_book_.emplace(symbol, std::make_pair(roq::Layer{}, (*iter_2).second)).first;
    }
    callback((*iter).second.first, (*iter).second.second);
    return true;
  }

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const bool master_;
  // web socket
  core::web::Socket connection_;
  // buffers
  core::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, auth, currencies, instruments, quote, ticker;
  } profile_;
  struct {
    core::metrics::Latency ping, heartbeat;
  } latency_;
  // cache
  Shared &shared_;
  absl::flat_hash_set<std::string> all_currencies_;  // only used by master
  absl::flat_hash_map<std::string, double>
      all_symbols_;  // only used by master (value is multiplier)
  std::vector<std::string> symbols_;
  absl::flat_hash_map<std::string, std::pair<roq::Layer, double> > top_of_book_;
  absl::flat_hash_map<std::string, TradingStatus> trading_status_;
  // state
  bool ready_ = false;
  ConnectionStatus status_ = {};
  server::Download<WebSocketState> download_;
};

}  // namespace deribit
}  // namespace roq
