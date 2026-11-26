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

#include "roq/core/json/buffer_stack.hpp"

#include "roq/core/download_2.hpp"

#include "roq/core/limit/queue.hpp"

#include "roq/server.hpp"

#include "roq/server/stream.hpp"

#include "roq/deribit/gateway/account.hpp"
#include "roq/deribit/gateway/currencies_update.hpp"
#include "roq/deribit/gateway/latch.hpp"
#include "roq/deribit/gateway/shared.hpp"

#include "roq/deribit/protocol/json/parser.hpp"

namespace roq {
namespace deribit {
namespace gateway {

struct DropCopy final : public Base<DropCopy>, public server::Stream, public web::socket::Client::Handler, public protocol::json::Parser::Handler {
  struct Handler {};

  DropCopy(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  void operator()(Trace<CurrenciesUpdate> const &);

  void operator()(Trace<Latch> const &);

 protected:
  friend base_type;

  // server::Stream

  void operator()(metrics::Writer &) const override;

  void operator()(Trace<Start> const &) override;
  void operator()(Trace<Stop> const &) override;
  void operator()(Trace<Timer> const &) override;

  void operator()(Trace<ConnectionStatus> const &, std::string_view const &reason) override;

  // web::socket::Client::Handler

  void operator()(Trace<web::socket::Connected> const &) override;
  void operator()(Trace<web::socket::Disconnected> const &) override;
  void operator()(Trace<web::socket::Ready> const &) override;
  void operator()(Trace<web::socket::Close> const &) override;
  void operator()(Trace<web::socket::Latency> const &) override;
  void operator()(Trace<web::socket::Text> const &) override;
  void operator()(Trace<web::socket::Binary> const &) override;

  // core::Download

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

  int32_t download(Trace<State> const &);

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

  void send_login(TraceInfo const &);

  void subscribe_user_portfolio(std::span<std::string const> const &currencies);
  void subscribe_user_changes();
  void subscribe_user_orders();
  void subscribe_user_trades();

  void get_account_summary(std::span<std::string const> const &currencies);
  void get_user_trades_by_currency(TraceInfo const &, std::span<std::string const> const &currencies);

  void parse(TraceInfo const &, std::string_view const &message);

  void check_subscribe_queue(TraceInfo const &);

 private:
  [[maybe_unused]] Handler &handler_;
  //
  std::string const name_;
  std::unique_ptr<web::socket::Client> const connection_;
  //
  core::json::BufferStack decode_buffer_;
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse, auth;
  } profile_;
  struct {
    utils::metrics::Latency ping, heartbeat;
  } latency_;
  Account &account_;
  Shared &shared_;
  std::vector<std::string> currencies_;
  core::Download2<State> download_;
  bool can_download_ = false;
  bool download_trades_is_first_ = true;
  core::limit::Queue<std::string> subscribe_queue_;
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
