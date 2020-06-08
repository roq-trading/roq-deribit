/* Copyright (c) 2017-2020, Hans Erik Thrane */

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

class WebSocket final
    : public core::web::Socket::Handler,
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
    virtual void operator()(const WebSocket&) = 0;
    virtual void operator()(const json::Currencies&) = 0;
    virtual void operator()(const json::Instruments&) = 0;
    virtual void operator()(const json::Positions&) = 0;
    virtual void operator()(const json::Ticker&) = 0;
  };

  WebSocket(
      Handler& handler,
      const Config& config,
      Random& random,
      core::event::Base& base,
      core::event::DNSBase& dns_base,
      core::ssl::Context& ssl_context);

  WebSocket(WebSocket&&) = delete;
  WebSocket(const WebSocket&) = delete;

  bool ready() const;

  void close();

  void operator()(const server::StartEvent&);
  void operator()(const server::StopEvent&);
  void operator()(const server::TimerEvent&);

  void login();

  void get_currencies();
  void get_instruments(const std::string_view& currency);
  void get_positions(const std::string_view& currency);

  template <typename T>
  void subscribe_ticker(const roq::span<T>& symbols);

  template <typename T>
  void unsubscribe_ticker(const roq::span<T>& symbols);

  void operator()(metrics::Writer& writer);

 protected:
  void operator()(const core::web::Socket::Connected&) override;
  void operator()(const core::web::Socket::Disconnected&) override;
  void operator()(const core::web::Socket::Ready&) override;
  void operator()(const core::web::Socket::Close&) override;
  void operator()(const core::web::Socket::Latency&) override;
  void operator()(const core::web::Socket::Text&) override;

 private:
  void parse(const std::string_view& message);

  void operator()(
      const core::jsonrpc::Error& error,
      core::json::value_t& value) override;
  void operator()(
      const core::jsonrpc::Result& result,
      core::json::value_t& value) override;
  void operator()(
      const core::jsonrpc::Notification& notification,
      core::json::value_t& value) override;

  void operator()(const json::Auth& auth);

  void operator()(const json::Currencies& currencies);
  void operator()(const json::Instruments& instruments);
  void operator()(const json::Positions& positions);

  void operator()(const json::Ticker& ticker) override;

 private:
  Handler& _handler;
  // config
  const std::string _access_key;
  // authentication
  Random& _random;
  // web socket
  core::web::Socket _connection;
  // buffers
  core::utils::Buffer _decode_buffer;
  core::stack::Buffer<char, 32> _stack_buffer;
  // metrics
  struct {
    core::metrics::Counter
      disconnect;
  } _counter;
  struct {
    core::metrics::Profile
      parse,
      auth,
      currencies,
      instruments,
      positions,
      ticker;
  } _profile;
  struct {
    core::metrics::Latency
      ping,
      heartbeat;
  } _latency;
  // state
  bool _ready = false;
};

}  // namespace deribit
}  // namespace roq
