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

#include "roq/deribit/drop_copy_state.h"
#include "roq/deribit/security.h"
#include "roq/deribit/shared.h"

#include "roq/deribit/json/auth.h"
#include "roq/deribit/json/changes.h"
#include "roq/deribit/json/orders.h"
#include "roq/deribit/json/parser.h"
#include "roq/deribit/json/portfolio.h"
#include "roq/deribit/json/positions.h"
#include "roq/deribit/json/trades.h"

namespace roq {
namespace deribit {

class DropCopy final : public core::web::Socket::Handler,
                       public core::jsonrpc::Parser::Handler,
                       public json::Parser::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<StreamUpdate> &) = 0;
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

  void update_subscriptions(const roq::span<std::string> &currencies);

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

  uint32_t download(DropCopyState);

  void subscribe_portfolios(const roq::span<std::string> &currencies);
  void subscribe_changes();

  void get_account_summary(const roq::span<std::string> &currencies);
  void get_trades(const roq::span<std::string> &currencies);
  void get_positions(const roq::span<std::string> &currencies);

  void parse(const std::string_view &message);

  void operator()(const core::jsonrpc::Error &, core::json::value_t &) override;
  void operator()(const core::jsonrpc::Result &, core::json::value_t &) override;
  void operator()(const core::jsonrpc::Notification &, core::json::value_t &) override;

  void operator()(const json::Auth &, const server::TraceInfo &);

 public:
  void operator()(const server::Trace<json::PlatformState> &) override;
  void operator()(const server::Trace<json::InstrumentState> &) override;
  void operator()(const server::Trace<json::Quote> &) override;
  void operator()(const server::Trace<json::Ticker> &) override;
  void operator()(const server::Trace<json::Portfolio> &) override;
  void operator()(const server::Trace<json::Changes> &) override;

  void operator()(const server::Trace<json::Trades> &);
  void operator()(const server::Trace<json::Positions> &);
  void operator()(const server::Trace<json::Orders> &);

  void operator()(const server::Trace<json::Trade> &, bool is_last);
  void operator()(const server::Trace<json::Position> &, bool is_last);

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
    core::metrics::Profile parse, auth, positions;
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
  GatewayStatus status_ = {};
  server::Download<DropCopyState> download_;
};

}  // namespace deribit
}  // namespace roq
