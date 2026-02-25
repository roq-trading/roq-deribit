/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "roq/utils/container.hpp"

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/core/download.hpp"
#include "roq/core/timer_queue.hpp"

#include "roq/server.hpp"

#include "roq/deribit/account.hpp"
#include "roq/deribit/request.hpp"
#include "roq/deribit/shared.hpp"
#include "roq/deribit/web_socket_state.hpp"

#include "roq/deribit/json/parser.hpp"

namespace roq {
namespace deribit {

struct WebSocket final : public web::socket::Client::Handler, public json::Parser::Handler {
  struct Latch final {};

  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<ReferenceData> const &, bool is_last) = 0;
    virtual void operator()(Trace<TopOfBook> const &, bool is_last) = 0;
    virtual void operator()(Trace<MarketStatus> const &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(Latch const &) = 0;
  };

  WebSocket(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &, Request &, size_t index, bool master);

  WebSocket(WebSocket const &) = delete;

  bool ready() const { return ready_; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  void subscribe(size_t start_from = 0);

 protected:
  // web::socket::Client::Handler

  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

 private:
  void operator()(ConnectionStatus, std::string_view const &reason = {});

  void login();

  uint32_t download(WebSocketState);

  void download_currencies();
  void check_currencies();

  void download_instruments();
  void check_instruments();

  void subscribe_platform_state();
  void subscribe_instrument_state();

  void subscribe(std::span<Symbol const> const &symbols);

  void subscribe_quote(std::span<Symbol const> const &symbols);
  void subscribe_ticker(std::span<Symbol const> const &symbols);
  void subscribe_chart_trades(std::span<Symbol const> const &symbols);

  void parse(std::string_view const &message);

  // json::Parser::Handler

  void operator()(Trace<json::Auth> const &) override;
  void operator()(Trace<json::SubscribeAck> const &) override;
  // public:
  void operator()(Trace<json::PlatformState> const &) override;
  void operator()(Trace<json::InstrumentState> const &) override;
  void operator()(Trace<json::Quote> const &) override;
  void operator()(Trace<json::Ticker> const &) override;
  void operator()(Trace<json::ChartTrades> const &, std::string_view const &symbol, uint32_t interval) override;
  // private:
  void operator()(Trace<json::UserPortfolio> const &) override;
  void operator()(Trace<json::UserChanges> const &) override;
  void operator()(Trace<json::UserOrders> const &) override;
  void operator()(Trace<json::UserTrades> const &) override;

  void operator()(Trace<json::GetAccountSummaryAck> const &) override;
  void operator()(Trace<json::GetUserTradesByCurrencyAck> const &) override;

  template <typename C>
  bool get_top_of_book(std::string_view const &symbol, C callback) {
    auto iter = top_of_book_.find(symbol);
    if (iter == std::end(top_of_book_)) {
      auto iter_2 = shared_.multiplier.find(symbol);
      if (iter_2 == std::end(shared_.multiplier)) {
        return false;
      }
      iter = top_of_book_.emplace(symbol, std::make_pair(roq::Layer{}, (*iter_2).second)).first;
    }
    callback((*iter).second.first, (*iter).second.second);
    return true;
  }

  void check_subscribe_queue(std::chrono::nanoseconds now);

  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  size_t const index_;
  bool const master_;
  bool const publish_top_of_book_;
  Mask<SupportType> const supports_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse, auth, currencies, instruments, quote, ticker, chart_trades;
  } profile_;
  struct {
    utils::metrics::Latency ping, heartbeat;
  } latency_;
  // account
  Account &account_;
  // cache
  Shared &shared_;
  utils::unordered_map<std::string, std::pair<roq::Layer, double>> top_of_book_;
  utils::unordered_map<std::string, TradingStatus> trading_status_;
  // state
  bool ready_ = false;
  ConnectionStatus connection_status_ = {};
  core::Download<WebSocketState> download_;
  // queue
  core::TimerQueue<std::string> subscribe_queue_;
  //
  Request &request_;
};

}  // namespace deribit
}  // namespace roq
