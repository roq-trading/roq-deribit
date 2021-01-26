/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/event/base.h"
#include "roq/core/event/dns_base.h"

#include "roq/core/ssl/ssl.h"

#include "roq/core/web/socket.h"

#include "roq/core/jsonrpc/parser.h"

#include "roq/server.h"

#include "roq/deribit/config.h"
#include "roq/deribit/random.h"

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
  enum class State {
    DISCONNECTED,
    LOGON_SENT,
    DOWNLOAD,
    READY,
  };

 public:
  struct Handler {
    virtual void operator()(const WebSocket &) = 0;
    virtual void operator()(const ExternalLatency &, const server::TraceInfo &) = 0;
    virtual void operator()(const json::Currencies &, const server::TraceInfo &) = 0;
    virtual void operator()(const json::Instruments &, const server::TraceInfo &) = 0;
    virtual void operator()(const json::Positions &, const server::TraceInfo &) = 0;
    virtual void operator()(const json::Ticker &, const server::TraceInfo &) = 0;
  };

  WebSocket(
      Handler &handler,
      const Config &config,
      Random &random,
      core::event::Base &base,
      core::event::DNSBase &dns_base,
      core::ssl::Context &ssl_context);

  WebSocket(WebSocket &&) = delete;
  WebSocket(const WebSocket &) = delete;

  bool ready() const;

  void close();

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void login();

  void get_currencies();
  void get_instruments(const std::string_view &currency);
  void get_positions(const std::string_view &currency);

  template <typename T>
  void subscribe_ticker(const roq::span<T> &symbols);

  template <typename T>
  void unsubscribe_ticker(const roq::span<T> &symbols);

  void operator()(metrics::Writer &writer);

 protected:
  void operator()(const core::web::Socket::Connected &) override;
  void operator()(const core::web::Socket::Disconnected &) override;
  void operator()(const core::web::Socket::Ready &) override;
  void operator()(const core::web::Socket::Close &) override;
  void operator()(const core::web::Socket::Latency &) override;
  void operator()(const core::web::Socket::Text &) override;

 private:
  void parse(const std::string_view &message);

  void operator()(const core::jsonrpc::Error &error, core::json::value_t &value) override;
  void operator()(const core::jsonrpc::Result &result, core::json::value_t &value) override;
  void operator()(
      const core::jsonrpc::Notification &notification, core::json::value_t &value) override;

  void operator()(const json::Auth &auth, const server::TraceInfo &trace_info);

  void operator()(const json::Currencies &currencies, const server::TraceInfo &trace_info);
  void operator()(const json::Instruments &instruments, const server::TraceInfo &trace_info);
  void operator()(const json::Positions &positions, const server::TraceInfo &trace_info);

  void operator()(const json::Ticker &ticker, const server::TraceInfo &trace_info) override;

 private:
  Handler &handler_;
  // config
  const std::string access_key_;
  // authentication
  Random &random_;
  // web socket
  core::web::Socket connection_;
  // buffers
  core::utils::Buffer decode_buffer_;
  // core::stack::Buffer<char, 32> stack_buffer_;
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
  // state
  bool ready_ = false;
};

}  // namespace deribit
}  // namespace roq
