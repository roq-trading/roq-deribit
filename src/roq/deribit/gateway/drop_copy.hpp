/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/core/timer_queue.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/core/download.hpp"

#include "roq/server.hpp"

#include "roq/deribit/gateway/account.hpp"
#include "roq/deribit/gateway/shared.hpp"

#include "roq/deribit/protocol/json/parser.hpp"

namespace roq {
namespace deribit {
namespace gateway {

struct DropCopy final : public web::socket::Client::Handler, public protocol::json::Parser::Handler {
  struct Handler {};

  DropCopy(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  DropCopy(DropCopy const &) = delete;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  void update_subscriptions(std::span<std::string> const &currencies);

  void download();

 protected:
  // web::socket::Client::Handler

  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

  // helpers

  void operator()(ConnectionStatus, std::string_view const &reason = {});

  void login();

  enum class State {
    UNDEFINED = 0,
    SUBSCRIBE_USER_PORTFOLIO,
    SUBSCRIBE_USER_CHANGES,
    SUBSCRIBE_USER_ORDERS,
    SUBSCRIBE_USER_TRADES,
    GET_ACCOUNT_SUMMARY,
    GET_USER_TRADES_BY_CURRENCY,
    DONE,
  };

  uint32_t download(State);

  void subscribe_user_portfolio(std::span<std::string> const &currencies);
  void subscribe_user_changes();
  void subscribe_user_orders();
  void subscribe_user_trades();

  void get_account_summary(std::span<std::string> const &currencies);
  void get_user_trades_by_currency(std::span<std::string> const &currencies);

  void parse(std::string_view const &message);

  // protocol::json::Parser::Handler

  void operator()(Trace<protocol::json::Auth> const &) override;
  void operator()(Trace<protocol::json::SubscribeAck> const &) override;

  void operator()(Trace<protocol::json::PlatformState> const &) override;
  void operator()(Trace<protocol::json::InstrumentState> const &) override;
  void operator()(Trace<protocol::json::Quote> const &) override;
  void operator()(Trace<protocol::json::Ticker> const &) override;
  void operator()(Trace<protocol::json::ChartTrades> const &, std::string_view const &symbol, uint32_t interval) override;

  void operator()(Trace<protocol::json::UserPortfolio> const &) override;
  void operator()(Trace<protocol::json::UserChanges> const &) override;
  void operator()(Trace<protocol::json::UserOrders> const &) override;
  void operator()(Trace<protocol::json::UserTrades> const &) override;

  void operator()(Trace<protocol::json::GetAccountSummaryAck> const &) override;
  void operator()(Trace<protocol::json::GetUserTradesByCurrencyAck> const &) override;

  void operator()(Trace<protocol::json::Trade> const &, bool is_download, bool is_last);

  // helpers

  void check_subscribe_queue(std::chrono::nanoseconds now);

 private:
  [[maybe_unused]] Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse, auth;
  } profile_;
  struct {
    utils::metrics::Latency ping, heartbeat;
  } latency_;
  // account
  Account &account_;
  // cache
  Shared &shared_;
  std::vector<std::string> currencies_;
  // state
  bool ready_ = false;
  ConnectionStatus connection_status_ = {};
  core::Download<State> download_;
  bool can_download_ = false;
  bool download_trades_is_first_ = true;
  //
  core::TimerQueue<std::string> subscribe_queue_;
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
