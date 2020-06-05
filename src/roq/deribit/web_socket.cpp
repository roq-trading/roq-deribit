/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/web_socket.h"

#include <fmt/format.h>

#include "roq/builtins.h"

#include "roq/core/clock.h"

#include "roq/deribit/options.h"

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
    Handler& handler,
    const Config& config,
    Random& random,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    core::ssl::Context& ssl_context)
    : _handler(handler),
      _access_key(config.get_access_key()),
      _random(random),
      _connection(
          *this,
          base,
          dns_base,
          ssl_context,
          core::URI(FLAGS_ws_uri),
          std::string_view(),  // query
          std::chrono::seconds { FLAGS_ws_ping_freq_secs },
          FLAGS_decode_buffer_size,  // XXX need read buffer size
          FLAGS_encode_buffer_size,
          []() { return std::string(); }),
      _decode_buffer(FLAGS_decode_buffer_size),
      _counter {
        .disconnect = create_counter("disconnect"),
      },
      _profile {
        .parse = create_profile("parse"),
        .auth = create_profile("auth"),
        .currencies = create_profile("currencies"),
        .instruments = create_profile("instruments"),
        .positions = create_profile("positions"),
        .ticker = create_profile("ticker"),
      },
      _latency {
        .ping = create_latency("ping"),
        .heartbeat = create_latency("heartbeat"),
      } {
}

bool WebSocket::ready() const {
  return _connection.ready();
}

void WebSocket::close() {
  _connection.close();
}

void WebSocket::operator()(const server::StartEvent&) {
  _connection.start();
}

void WebSocket::operator()(const server::StopEvent&) {
  _connection.stop();
}

void WebSocket::operator()(const server::TimerEvent& event) {
  _connection.refresh(event.now);
}

void WebSocket::login() {
  constexpr json::RequestType request_type =
    json::RequestType::AUTH;
  auto timestamp = std::chrono::duration_cast<
    std::chrono::milliseconds>(core::get_realtime_clock());
  auto nonce = _random.create_nonce();
  auto signature = _random.create_signature(
      timestamp,
      nonce);
  auto message = fmt::format(
      FMT_STRING(
        R"({{)"
        R"("method":"public/auth",)"
        R"("params":{{)"
        R"("grant_type":"client_signature",)"
        R"("client_id":"{}",)"
        R"("timestamp":"{}",)"
        R"("nonce":"{}",)"
        R"("data":"",)"
        R"("signature":"{}")"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"),
      _access_key,
      timestamp.count(),
      nonce,
      signature,
      request_type.as_raw_text());
  _connection.send_text(message);
}

void WebSocket::get_currencies() {
  constexpr json::RequestType request_type =
    json::RequestType::GET_CURRENCIES;
  auto message = fmt::format(
      FMT_STRING(
        R"({{)"
        R"("method":"public/get_currencies",)"
        R"("params":{{}},)"
        R"("id":"{}")"
        R"(}})"),
      request_type.as_raw_text());
  _connection.send_text(message);
}

void WebSocket::get_instruments(const std::string_view& currency) {
  constexpr json::RequestType request_type =
    json::RequestType::GET_INSTRUMENTS;
  auto message = fmt::format(
      FMT_STRING(
        R"({{)"
        R"("method":"public/get_instruments",)"
        R"("params":{{)"
        R"("currency":"{}")"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"),
      currency,
      request_type.as_raw_text());
  _connection.send_text(message);
}

void WebSocket::get_positions(const std::string_view& currency) {
  constexpr json::RequestType request_type =
    json::RequestType::GET_POSITIONS;
  auto message = fmt::format(
      FMT_STRING(
        R"({{)"
        R"("method":"private/get_positions",)"
        R"("params":{{)"
        R"("currency":"{}")"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"),
      currency,
      request_type.as_raw_text());
  _connection.send_text(message);
}

template <typename T>
void WebSocket::subscribe_ticker(
    const roq::span<T>& symbols) {
  constexpr json::RequestType request_type =
    json::RequestType::SUBSCRIBE_TICKER;
  auto message = fmt::format(
      FMT_STRING(
        R"({{)"
        R"("method":"public/subscribe",)"
        R"("params":{{)"
        R"("channels":["ticker.{}.raw"])"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"),
      fmt::join(
          symbols,
          R"(.raw","ticker.)"),
      request_type.as_raw_text());
  _connection.send_text(message);
}

template
void WebSocket::subscribe_ticker(const roq::span<std::string>&);

template
void WebSocket::subscribe_ticker(const roq::span<std::string_view>&);

template <typename T>
void WebSocket::unsubscribe_ticker(
    const roq::span<T>& symbols) {
  constexpr json::RequestType request_type =
    json::RequestType::UNSUBSCRIBE_TICKER;
  auto message = fmt::format(
      FMT_STRING(
        R"({{)"
        R"("method":"public/unsubscribe",)"
        R"("params":{{)"
        R"("channels":["ticker.{}.raw"])"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"),
      fmt::join(
          symbols,
          R"(.raw","ticker.)"),
      request_type.as_raw_text());
  _connection.send_text(message);
}

template
void WebSocket::unsubscribe_ticker(const roq::span<std::string>&);

template
void WebSocket::unsubscribe_ticker(const roq::span<std::string_view>&);

void WebSocket::operator()(Metrics& metrics) {
  metrics
    // counter
    .write(_counter.disconnect)
    // profile
    .write(_profile.parse)
    .write(_profile.auth)
    .write(_profile.currencies)
    .write(_profile.instruments)
    .write(_profile.positions)
    .write(_profile.ticker)
    // latency
    .write(_latency.ping)
    .write(_latency.heartbeat);
}

void WebSocket::operator()(const core::web::Socket::Connected&) {
  // note! wait for upgrade
}

void WebSocket::operator()(const core::web::Socket::Disconnected&) {
  ++_counter.disconnect;
  _ready = false;
  _handler(*this);
}

void WebSocket::operator()(const core::web::Socket::Ready&) {
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
          core::jsonrpc::Parser::dispatch(
              *this,
              message);
        } catch (std::exception& e) {
          LOG(WARNING)(
              FMT_STRING(R"(message="{}")"),
              message);
          LOG(FATAL)(
              FMT_STRING(R"("ERROR what="{}")"),
              e.what());
        }
      });
}

void WebSocket::operator()(
    const core::jsonrpc::Error& error,
    core::json::value_t& value) {
  json::Error error_2(value);
  LOG(FATAL)(
      FMT_STRING(R"(error={}, id="{}")"),
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
          FMT_STRING(R"(Unknown request_type="{}")"),
          result.id);
      break;
    case json::RequestType::AUTH: {
      json::Auth auth(value);
      (*this)(auth);
      break;
    }
    case json::RequestType::GET_CURRENCIES: {
      core::json::Buffer buffer(_decode_buffer);
      json::Currencies currencies(value, buffer);
      (*this)(currencies);
      break;
    }
    case json::RequestType::GET_INSTRUMENTS: {
      core::json::Buffer buffer(_decode_buffer);
      json::Instruments instruments(value, buffer);
      (*this)(instruments);
      break;
    }
    case json::RequestType::GET_POSITIONS: {
      core::json::Buffer buffer(_decode_buffer);
      json::Positions positions(value, buffer);
      (*this)(positions);
      break;
    }
    case json::RequestType::SUBSCRIBE_TICKER:
    case json::RequestType::UNSUBSCRIBE_TICKER:
      break;
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
          FMT_STRING(R"(Unknown method="{}")"),
          notification.method);
      break;
    case json::Method::SUBSCRIPTION: {
      core::json::Buffer buffer(_decode_buffer);
      json::Parser::dispatch(
          *this,
          value,
          buffer);
      break;
    }
  }
}

void WebSocket::operator()(const json::Auth& auth) {
  _profile.auth(
      [&]() {
    VLOG(1)(
        FMT_STRING(R"(auth={})"),
        auth);
    LOG(INFO)("Ready");
    assert(_ready == false);
    _ready = true;
    _handler(*this);
  });
}

void WebSocket::operator()(const json::Currencies& currencies) {
  _profile.currencies(
      [&]() {
    VLOG(1)(
        FMT_STRING(R"(currencies={})"),
        currencies);
    _handler(currencies);
  });
}

void WebSocket::operator()(const json::Instruments& instruments) {
  _profile.instruments(
      [&]() {
    VLOG(1)(
        FMT_STRING(R"(instruments={})"),
        instruments);
    _handler(instruments);
  });
}

void WebSocket::operator()(const json::Positions& positions) {
  _profile.positions(
      [&]() {
    VLOG(1)(
        FMT_STRING(R"(positions={})"),
        positions);
    _handler(positions);
  });
}

void WebSocket::operator()(const json::Ticker& ticker) {
  _profile.ticker(
      [&]() {
    VLOG(2)(
        FMT_STRING(R"(ticker={})"),
        ticker);
    _handler(ticker);
  });
}

}  // namespace deribit
}  // namespace roq
