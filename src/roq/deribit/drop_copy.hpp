/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "roq/core/download.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/web/client_socket.hpp"

#include "roq/core/jsonrpc/parser.hpp"

#include "roq/server.hpp"

#include "roq/deribit/drop_copy_state.hpp"
#include "roq/deribit/security.hpp"
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

class DropCopy final : public core::web::ClientSocket::Handler,
                       public core::jsonrpc::Parser::Handler,
                       public json::Parser::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<FundsUpdate> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<PositionUpdate> &, bool is_last) = 0;
  };

  DropCopy(Handler &, core::io::Context &, uint16_t stream_id, Security &, Shared &);

  DropCopy(DropCopy &&) = delete;
  DropCopy(const DropCopy &) = delete;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  void update_subscriptions(const std::span<std::string> &currencies);

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

  void login();

  uint32_t download(DropCopyState);

  void subscribe_portfolios(const std::span<std::string> &currencies);
  void subscribe_changes();
  void subscribe_orders();
  void subscribe_trades();

  void get_account_summary(const std::span<std::string> &currencies);
  void get_trades(const std::span<std::string> &currencies);

  void parse(const std::string_view &message);

  void operator()(const server::Trace<core::jsonrpc::Error> &, core::json::value_t &) override;
  void operator()(const server::Trace<core::jsonrpc::Result> &, core::json::value_t &) override;
  void operator()(
      const server::Trace<core::jsonrpc::Notification> &, core::json::value_t &) override;

  void operator()(const server::Trace<json::Auth> &);

 public:
  void operator()(const server::Trace<json::PlatformState> &) override;
  void operator()(const server::Trace<json::InstrumentState> &) override;
  void operator()(const server::Trace<json::Quote> &) override;
  void operator()(const server::Trace<json::Ticker> &) override;
  void operator()(const server::Trace<json::Portfolio> &) override;
  void operator()(const server::Trace<json::Changes> &) override;

  void operator()(const server::Trace<json::Trades> &);
  void operator()(const server::Trace<json::Positions> &);
  void operator()(const server::Trace<json::Order> &) override;
  void operator()(const server::Trace<json::Trades2> &) override;

  void operator()(const server::Trace<json::Trade> &, bool is_last);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  // web socket
  core::web::ClientSocket connection_;
  // buffers
  core::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, auth;
  } profile_;
  struct {
    core::metrics::Latency ping, heartbeat;
  } latency_;
  // security
  Security &security_;
  // cache
  Shared &shared_;
  std::vector<std::string> currencies_;
  // state
  bool ready_ = false;
  ConnectionStatus status_ = {};
  core::Download<DropCopyState> download_;
};

}  // namespace deribit
}  // namespace roq
