/* Copyright (c) 2017-2025, Hans Erik Thrane */

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

#include "roq/core/jsonrpc/parser.hpp"

#include "roq/core/download.hpp"

#include "roq/server.hpp"

#include "roq/deribit/account.hpp"
#include "roq/deribit/drop_copy_state.hpp"
#include "roq/deribit/shared.hpp"

#include "roq/deribit/json/auth.hpp"
#include "roq/deribit/json/changes.hpp"
#include "roq/deribit/json/order.hpp"
#include "roq/deribit/json/parser.hpp"
#include "roq/deribit/json/portfolio.hpp"
#include "roq/deribit/json/positions.hpp"
#include "roq/deribit/json/trades.hpp"

namespace roq {
namespace deribit {

struct DropCopy final : public web::socket::Client::Handler, public core::jsonrpc::Parser::Handler, public json::Parser::Handler {
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
  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

 private:
  void operator()(ConnectionStatus);

  void login();

  uint32_t download(DropCopyState);

  void subscribe_portfolios(std::span<std::string> const &currencies);
  void subscribe_changes();
  void subscribe_orders();
  void subscribe_trades();

  void get_account_summary(std::span<std::string> const &currencies);
  void get_trades(std::span<std::string> const &currencies);

  void parse(std::string_view const &message);

  void operator()(Trace<core::jsonrpc::Error> const &, core::json::Value &) override;
  bool operator()(Trace<core::jsonrpc::Result> const &, core::json::Value &) override;
  bool operator()(Trace<core::jsonrpc::Notification> const &, core::json::Value &) override;

  void operator()(Trace<json::Auth> const &);

 public:
  void operator()(Trace<json::PlatformState> const &) override;
  void operator()(Trace<json::InstrumentState> const &) override;
  void operator()(Trace<json::Quote> const &) override;
  void operator()(Trace<json::Ticker> const &) override;
  void operator()(Trace<json::ChartTrades> const &, std::string_view const &symbol, uint32_t interval) override;
  void operator()(Trace<json::Portfolio> const &) override;
  void operator()(Trace<json::Changes> const &) override;

  void operator()(Trace<json::Trades> const &);
  void operator()(Trace<json::Positions> const &);
  void operator()(Trace<json::Order> const &) override;
  void operator()(Trace<json::Trades2> const &) override;

  void operator()(Trace<json::Trade> const &, bool is_download, bool is_last);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  std::vector<std::byte> decode_buffer_;
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
  ConnectionStatus status_ = {};
  core::Download<DropCopyState> download_;
  bool can_download_ = false;
  bool download_trades_is_first_ = true;
};

}  // namespace deribit
}  // namespace roq
