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

#include "roq/deribit/config.h"
#include "roq/deribit/random.h"

namespace roq {
namespace deribit {

class Gateway;

class WebSocket final
    : public core::web::Socket::Handler,
      public core::jsonrpc::Parser::Handler {

 public:
  WebSocket(
      Gateway& gateway,
      const Config& config,
      Random& random,
      core::event::Base& base,
      core::event::DNSBase& dns_base,
      core::ssl::Context& ssl_context);

  WebSocket(WebSocket&&) = delete;
  WebSocket(const WebSocket&) = delete;

  bool ready() const;

  void close();

  void operator()(const StartEvent&);
  void operator()(const StopEvent&);
  void operator()(const TimerEvent&);

  // request

  void login();

  void operator()(Metrics& metrics);

 protected:
  void operator()(const core::web::Socket::Connected&) override;
  void operator()(const core::web::Socket::Disconnected&) override;
  void operator()(const core::web::Socket::Ready&) override;
  void operator()(const core::web::Socket::Close&) override;
  void operator()(const core::web::Socket::Latency&) override;
  void operator()(const core::web::Socket::Text&) override;

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

 private:
  Gateway& _gateway;
  // config
  const std::string _access_key;
  // authentication
  Random& _random;
  // web socket
  core::web::Socket _connection;
  // buffers
  core::utils::Buffer _decode_buffer;
  // session
  std::chrono::nanoseconds _next_heartbeat = {};
  bool _logged_in = false;
  // other
  core::stack::Buffer<char, 32> _stack_buffer;
  // metrics
  struct {
    core::metrics::Counter
      disconnect;
  } _counter;
  struct {
    core::metrics::Profile
      parse;
  } _profile;
  struct {
    core::metrics::Latency
      ping,
      heartbeat;
  } _latency;
};

}  // namespace deribit
}  // namespace roq
