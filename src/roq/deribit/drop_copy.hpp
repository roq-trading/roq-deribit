/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/core/download.hpp"

#include "roq/server.hpp"

#include "roq/deribit/account.hpp"
#include "roq/deribit/drop_copy_state.hpp"
#include "roq/deribit/shared.hpp"

#include "roq/deribit/json/parser.hpp"

namespace roq {
namespace deribit {

struct DropCopy final : public web::socket::Client::Handler, public json::Parser::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<TradeUpdate> const &, bool is_last, uint8_t user_id, std::string_view const &request_id) = 0;
    virtual void operator()(Trace<FundsUpdate> const &, bool is_last) = 0;
    virtual void operator()(Trace<PositionUpdate> const &, bool is_last) = 0;
  };

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

 private:
  void operator()(ConnectionStatus, std::string_view const &reason = {});

  void login();

  uint32_t download(DropCopyState);

  void subscribe_user_portfolio(std::span<std::string> const &currencies);
  void subscribe_user_changes();
  void subscribe_user_orders();
  void subscribe_user_trades();

  void get_account_summary(std::span<std::string> const &currencies);
  void get_user_trades_by_currency(std::span<std::string> const &currencies);

  void parse(std::string_view const &message);

 public:
  // json::Parser::Handler

  void operator()(Trace<json::Auth> const &) override;
  void operator()(Trace<json::SubscribeAck> const &) override;

  void operator()(Trace<json::PlatformState> const &) override;
  void operator()(Trace<json::InstrumentState> const &) override;
  void operator()(Trace<json::Quote> const &) override;
  void operator()(Trace<json::Ticker> const &) override;
  void operator()(Trace<json::ChartTrades> const &, std::string_view const &symbol, uint32_t interval) override;

  void operator()(Trace<json::UserPortfolio> const &) override;
  void operator()(Trace<json::UserChanges> const &) override;
  void operator()(Trace<json::UserOrders> const &) override;
  void operator()(Trace<json::UserTrades> const &) override;

  void operator()(Trace<json::GetAccountSummaryAck> const &) override;
  void operator()(Trace<json::GetUserTradesByCurrencyAck> const &) override;

  void operator()(Trace<json::Trade> const &, bool is_download, bool is_last);

 private:
  Handler &handler_;
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
  core::Download<DropCopyState> download_;
  bool can_download_ = false;
  bool download_trades_is_first_ = true;
};

}  // namespace deribit
}  // namespace roq
