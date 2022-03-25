/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "roq/core/download.hpp"
#include "roq/core/timer_queue.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/web/client_socket.hpp"

#include "roq/core/jsonrpc/parser.hpp"

#include "roq/server.hpp"

#include "roq/deribit/security.hpp"
#include "roq/deribit/shared.hpp"
#include "roq/deribit/web_socket_state.hpp"

#include "roq/deribit/json/auth.hpp"
#include "roq/deribit/json/currencies.hpp"
#include "roq/deribit/json/instruments.hpp"
#include "roq/deribit/json/parser.hpp"
#include "roq/deribit/json/positions.hpp"
#include "roq/deribit/json/ticker.hpp"

namespace roq {
namespace deribit {

class WebSocket final : public core::web::ClientSocket::Handler,
                        public core::jsonrpc::Parser::Handler,
                        public json::Parser::Handler {
 public:
  struct CurrenciesUpdate final {
    std::vector<std::string> &currencies;
  };
  struct SymbolsUpdate final {
    std::vector<Symbol> &symbols;
  };

  struct Handler {
    virtual void operator()(const Trace<StreamStatus> &) = 0;
    virtual void operator()(const Trace<ExternalLatency> &) = 0;
    virtual void operator()(const Trace<TopOfBook> &, bool is_last) = 0;
    virtual void operator()(const Trace<MarketStatus> &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(CurrenciesUpdate &) = 0;
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  WebSocket(
      Handler &, core::io::Context &, uint16_t stream_id, Shared &, size_t index, bool master);

  WebSocket(WebSocket &&) = delete;
  WebSocket(const WebSocket &) = delete;

  bool ready() const { return ready_; }

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  void subscribe(size_t start_from = 0);

 protected:
  void operator()(const core::web::ClientSocket::Connected &) override;
  void operator()(const core::web::ClientSocket::Disconnected &) override;
  void operator()(const core::web::ClientSocket::Ready &) override;
  void operator()(const core::web::ClientSocket::Close &) override;
  void operator()(const core::web::ClientSocket::Latency &) override;
  void operator()(const core::web::ClientSocket::Text &) override;
  void operator()(const core::web::ClientSocket::Binary &) override;

 private:
  void operator()(ConnectionStatus);

  uint32_t download(WebSocketState);

  uint32_t download_currencies();
  uint32_t download_instruments();

  void get_currencies();
  void get_instruments(const std::string_view &currency);

  void subscribe_platform_state();
  void subscribe_instrument_state();

  void subscribe(const std::span<Symbol const> &symbols);

  void subscribe_quote(const std::span<Symbol const> &symbols);
  void subscribe_ticker(const std::span<Symbol const> &symbols);

  void parse(const std::string_view &message);

  void operator()(const Trace<core::jsonrpc::Error> &, core::json::value_t &) override;
  void operator()(const Trace<core::jsonrpc::Result> &, core::json::value_t &) override;
  void operator()(const Trace<core::jsonrpc::Notification> &, core::json::value_t &) override;

  void operator()(const Trace<json::Auth> &);

  void operator()(const Trace<json::Currencies> &);
  void operator()(const Trace<json::Instruments> &);
  void operator()(const Trace<json::Positions> &);

  // public:
  void operator()(const Trace<json::PlatformState> &) override;
  void operator()(const Trace<json::InstrumentState> &) override;
  void operator()(const Trace<json::Quote> &) override;
  void operator()(const Trace<json::Ticker> &) override;
  // private:
  void operator()(const Trace<json::Portfolio> &) override;
  void operator()(const Trace<json::Changes> &) override;
  void operator()(const Trace<json::Order> &) override;
  void operator()(const Trace<json::Trades2> &) override;

  template <typename C>
  bool get_top_of_book(const std::string_view &symbol, C callback) {
    auto iter = top_of_book_.find(symbol);
    if (iter == std::end(top_of_book_)) {
      auto iter_2 = shared_.multiplier.find(symbol);
      if (iter_2 == std::end(shared_.multiplier))
        return false;
      iter = top_of_book_.emplace(symbol, std::make_pair(roq::Layer{}, (*iter_2).second)).first;
    }
    callback((*iter).second.first, (*iter).second.second);
    return true;
  }

  void check_subscribe_queue(std::chrono::nanoseconds now);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const size_t index_;
  const bool master_;
  // web socket
  core::web::ClientSocket connection_;
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
  absl::flat_hash_map<std::string, std::pair<roq::Layer, double> > top_of_book_;
  absl::flat_hash_map<std::string, TradingStatus> trading_status_;
  // state
  bool ready_ = false;
  ConnectionStatus status_ = {};
  core::Download<WebSocketState> download_;
  // queue
  core::TimerQueue subscribe_queue_;
};

}  // namespace deribit
}  // namespace roq
