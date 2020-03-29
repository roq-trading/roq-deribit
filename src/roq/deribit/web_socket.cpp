/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/web_socket.h"

#include <fmt/format.h>

#include "roq/builtins.h"

#include "roq/core/clock.h"

#include "roq/deribit/gateway.h"
#include "roq/deribit/options.h"

#include "roq/deribit/json/auth_response.h"
#include "roq/deribit/json/error.h"
#include "roq/deribit/json/method.h"
#include "roq/deribit/json/request_type.h"

namespace roq {
namespace deribit {

namespace {
constexpr std::string_view CONNECTION = "ws";

static auto create_counter(
    const std::string_view& function) {
  return core::metrics::Counter(
      FLAGS_name,
      CONNECTION,
      function);
}

static auto create_profile(
    const std::string_view& function) {
  return core::metrics::Profile(
      FLAGS_name,
      CONNECTION,
      function);
}

static auto create_latency(
    const std::string_view& function) {
  return core::metrics::Latency(
      FLAGS_name,
      CONNECTION,
      function);
}
}  // namespace

WebSocket::WebSocket(
    Gateway& gateway,
    const Config& config,
    Random& random,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    core::ssl::Context& ssl_context)
    : _gateway(gateway),
      _access_key(config.get_access_key()),
      _random(random),
      _connection(
          *this,
          base,
          dns_base,
          ssl_context,
          core::URI(FLAGS_ws_uri),
          std::chrono::seconds { FLAGS_ping_freq_secs },
          FLAGS_decode_buffer_size,  // XXX need read buffer size
          FLAGS_encode_buffer_size,
          []() { return std::string(); }),
      _decode_buffer(FLAGS_decode_buffer_size),
      _counter {
        .disconnect = create_counter("disconnect"),
      },
      _profile {
        .parse = create_profile("parse"),
      },
      _latency {
        .ping = create_latency("ping"),
        .heartbeat = create_latency("heartbeat"),
      } {
}

bool WebSocket::ready() const {
  return _connection.ready() && _logged_in;
}

void WebSocket::close() {
  _connection.close();
}

void WebSocket::operator()(const StartEvent&) {
  _connection.start();
}

void WebSocket::operator()(const StopEvent&) {
  _connection.stop();
}

void WebSocket::operator()(const TimerEvent& event) {
  _connection.refresh(event.now);
}

void WebSocket::login() {
  LOG(INFO)("Sending login request...");
  constexpr json::RequestType request_type =
    json::RequestType::LOGIN;
  auto timestamp = std::chrono::duration_cast<
    std::chrono::milliseconds>(core::get_realtime_clock());
  auto nonce = _random.create_nonce();
  auto signature = _random.create_signature(
      timestamp,
      nonce);
  auto message = fmt::format(
      FMT_STRING(
        "{{"
        "\"method\":\"public/auth\","
        "\"params\":{{"
        "\"grant_type\":\"client_signature\","
        "\"client_id\":\"{}\","
        "\"timestamp\":\"{}\","
        "\"nonce\":\"{}\","
        "\"data\":\"\","
        "\"signature\":\"{}\""
        "}},"
        "\"id\":\"{}\""
        "}}"),
      _access_key,
      timestamp.count(),
      nonce,
      signature,
      request_type.as_raw_text());
  _connection.send_text(message);
}

void WebSocket::operator()(Metrics& metrics) {
  metrics
    // counter
    .write(_counter.disconnect)
    // profile
    .write(_profile.parse)
    // latency
    .write(_latency.ping)
    .write(_latency.heartbeat);
}

void WebSocket::operator()(const core::web::Socket::Connected&) {
  _gateway(*this);
}

void WebSocket::operator()(const core::web::Socket::Disconnected&) {
  _logged_in = false;
  _gateway(*this);
}

void WebSocket::operator()(const core::web::Socket::Ready&) {
  _gateway(*this);
  login();
}

void WebSocket::operator()(const core::web::Socket::Close&) {
}

void WebSocket::operator()(const core::web::Socket::Latency& latency) {
  _latency.ping.update(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          latency.sample).count());
}

void WebSocket::operator()(const core::web::Socket::Text& text) {
  parse(text.payload);
}

void WebSocket::parse(const std::string_view& message) {
  _profile.parse(
      [&]() {
        try {
          DLOG(INFO)(FMT_STRING("{}"), message);
          core::jsonrpc::Parser::dispatch(
              *this,
              message);
        } catch (std::exception& e) {
          LOG(FATAL)(
              FMT_STRING("ERROR what=\"{}\""),
              e.what());
        }
      });
}

void WebSocket::operator()(
    const core::jsonrpc::Error& error,
    core::json::value_t& value) {
  json::Error error_2(value);
  LOG(FATAL)(
      FMT_STRING("error={}, id=\"{}\""),
      error_2,
      error.id);
}

void WebSocket::operator()(
    const core::jsonrpc::Result& result,
    core::json::value_t& value) {
  json::RequestType request_type(result.id);
  switch (request_type) {
    case json::RequestType::UNDEFINED:
      break;
    case json::RequestType::UNKNOWN:
      DLOG(FATAL)(
          FMT_STRING("Unknown request_type=\"{}\""),
          result.id);
      break;
    case json::RequestType::LOGIN: {
      json::AuthResponse response(value);
      LOG(INFO)(FMT_STRING("{}"), response);
      LOG(INFO)("Login successful");
      _logged_in = true;
      _gateway(*this);
      break;
    }
  }
}

void WebSocket::operator()(
    const core::jsonrpc::Notification& notification,
    core::json::value_t& value) {
  json::Method method(notification.method);
  switch (method) {
    case json::Method::UNDEFINED:
      break;
    case json::Method::UNKNOWN:
      DLOG(FATAL)(
          FMT_STRING("Unknown method=\"{}\""),
          notification.method);
      break;
  }
}

}  // namespace deribit
}  // namespace roq
