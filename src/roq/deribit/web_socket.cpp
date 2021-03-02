/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/web_socket.h"

#include "roq/core/clock.h"

#include "roq/core/metrics/factory.h"

#include "roq/deribit/flags.h"

#include "roq/deribit/json/error.h"
#include "roq/deribit/json/method.h"
#include "roq/deribit/json/request_type.h"

using namespace roq::literals;

namespace roq {
namespace deribit {

namespace {
static const auto CONNECTION = "ws"_sv;

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &function)
      : core::metrics::Factory(Flags::name(), CONNECTION, function) {}
};
}  // namespace

WebSocket::WebSocket(Handler &handler, Security &security, core::io::Context &context)
    : handler_(handler), security_(security),
      connection_(
          *this,
          context,
          core::URI(Flags::ws_uri()),
          std::string_view(),  // query
          Flags::ws_ping_freq(),
          Flags::decode_buffer_size(),  // XXX need read buffer size
          Flags::encode_buffer_size(),
          []() { return std::string(); }),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics("disconnect"_sv),
      },
      profile_{
          .parse = create_metrics("parse"_sv),
          .auth = create_metrics("auth"_sv),
          .currencies = create_metrics("currencies"_sv),
          .instruments = create_metrics("instruments"_sv),
          .positions = create_metrics("positions"_sv),
          .ticker = create_metrics("ticker"_sv),
      },
      latency_{
          .ping = create_metrics("ping"_sv),
          .heartbeat = create_metrics("heartbeat"_sv),
      } {
}

bool WebSocket::ready() const {
  return connection_.ready();
}

void WebSocket::close() {
  connection_.close();
}

void WebSocket::operator()(const Event<Start> &) {
  connection_.start();
}

void WebSocket::operator()(const Event<Stop> &) {
  connection_.stop();
}

void WebSocket::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void WebSocket::login() {
  constexpr json::RequestType request_type = json::RequestType::AUTH;
  auto timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(core::get_realtime_clock());
  auto nonce = security_.create_nonce();
  auto signature = security_.create_signature(timestamp, nonce);
  auto message = roq::format(
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
      R"(}})"_fmt,
      security_.get_access_key(),
      timestamp.count(),
      nonce,
      signature,
      request_type.as_raw_text());
  connection_.send_text(message);
}

void WebSocket::get_currencies() {
  constexpr json::RequestType request_type = json::RequestType::GET_CURRENCIES;
  auto message = roq::format(
      R"({{)"
      R"("method":"public/get_currencies",)"
      R"("params":{{}},)"
      R"("id":"{}")"
      R"(}})"_fmt,
      request_type.as_raw_text());
  connection_.send_text(message);
}

void WebSocket::get_instruments(const std::string_view &currency) {
  constexpr json::RequestType request_type = json::RequestType::GET_INSTRUMENTS;
  auto message = roq::format(
      R"({{)"
      R"("method":"public/get_instruments",)"
      R"("params":{{)"
      R"("currency":"{}")"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_fmt,
      currency,
      request_type.as_raw_text());
  connection_.send_text(message);
}

void WebSocket::get_positions(const std::string_view &currency) {
  constexpr json::RequestType request_type = json::RequestType::GET_POSITIONS;
  auto message = roq::format(
      R"({{)"
      R"("method":"private/get_positions",)"
      R"("params":{{)"
      R"("currency":"{}")"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_fmt,
      currency,
      request_type.as_raw_text());
  connection_.send_text(message);
}

template <typename T>
void WebSocket::subscribe_ticker(const roq::span<T> &symbols) {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_TICKER;
  auto message = roq::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["ticker.{}.raw"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_fmt,
      roq::join(symbols, R"(.raw","ticker.)"_sv),
      request_type.as_raw_text());
  connection_.send_text(message);
}

template void WebSocket::subscribe_ticker(const roq::span<std::string> &);

template void WebSocket::subscribe_ticker(const roq::span<std::string_view> &);

template <typename T>
void WebSocket::unsubscribe_ticker(const roq::span<T> &symbols) {
  constexpr json::RequestType request_type = json::RequestType::UNSUBSCRIBE_TICKER;
  auto message = roq::format(
      R"({{)"
      R"("method":"public/unsubscribe",)"
      R"("params":{{)"
      R"("channels":["ticker.{}.raw"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_fmt,
      roq::join(symbols, R"(.raw","ticker.)"_sv),
      request_type.as_raw_text());
  connection_.send_text(message);
}

template void WebSocket::unsubscribe_ticker(const roq::span<std::string> &);

template void WebSocket::unsubscribe_ticker(const roq::span<std::string_view> &);

void WebSocket::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.auth, metrics::PROFILE)
      .write(profile_.currencies, metrics::PROFILE)
      .write(profile_.instruments, metrics::PROFILE)
      .write(profile_.positions, metrics::PROFILE)
      .write(profile_.ticker, metrics::PROFILE)
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void WebSocket::operator()(const core::web::Socket::Connected &) {
  // note! wait for upgrade
}

void WebSocket::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  handler_(*this);
}

void WebSocket::operator()(const core::web::Socket::Ready &) {
  login();
}

void WebSocket::operator()(const core::web::Socket::Close &) {
}

void WebSocket::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .stream_id = {},
      .name = CONNECTION,
      .latency = latency.sample,
  };
  handler_(external_latency, trace_info);
  latency_.ping.update(latency.sample);
}

void WebSocket::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void WebSocket::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      core::jsonrpc::Parser::dispatch(*this, message);
    } catch (std::exception &e) {
      LOG(WARNING)(R"(message="{}")"_fmt, message);
      LOG(FATAL)(R"("ERROR what="{}")"_fmt, e.what());
    }
  });
}

void WebSocket::operator()(const core::jsonrpc::Error &error, core::json::value_t &value) {
  json::Error error_2(value);
  LOG(FATAL)(R"(error={}, id="{}")"_fmt, error_2, error.id);
}

void WebSocket::operator()(const core::jsonrpc::Result &result, core::json::value_t &value) {
  server::TraceInfo trace_info;  // XXX not correct (*parsing* has already started)
  json::RequestType request_type(result.id);
  switch (request_type) {
    case json::RequestType::UNDEFINED:
      break;
    case json::RequestType::UNKNOWN:
      DLOG(FATAL)(R"(Unknown request_type="{}")"_fmt, result.id);
      break;
    case json::RequestType::AUTH: {
      json::Auth auth(value);
      (*this)(auth, trace_info);
      break;
    }
    case json::RequestType::GET_CURRENCIES: {
      core::json::Buffer buffer(decode_buffer_);
      json::Currencies currencies(value, buffer);
      (*this)(currencies, trace_info);
      break;
    }
    case json::RequestType::GET_INSTRUMENTS: {
      core::json::Buffer buffer(decode_buffer_);
      json::Instruments instruments(value, buffer);
      (*this)(instruments, trace_info);
      break;
    }
    case json::RequestType::GET_POSITIONS: {
      core::json::Buffer buffer(decode_buffer_);
      json::Positions positions(value, buffer);
      (*this)(positions, trace_info);
      break;
    }
    case json::RequestType::SUBSCRIBE_TICKER:
    case json::RequestType::UNSUBSCRIBE_TICKER:
      break;
  }
}

void WebSocket::operator()(
    const core::jsonrpc::Notification &notification, core::json::value_t &value) {
  server::TraceInfo trace_info;  // XXX not correct (*parsing* has already started)
  json::Method method(notification.method);
  switch (method) {
    case json::Method::UNDEFINED:
      break;
    case json::Method::UNKNOWN:
      DLOG(FATAL)(R"(Unknown method="{}")"_fmt, notification.method);
      break;
    case json::Method::SUBSCRIPTION: {
      core::json::Buffer buffer(decode_buffer_);
      json::Parser::dispatch(*this, value, buffer, trace_info);
      break;
    }
  }
}

void WebSocket::operator()(const json::Auth &auth, const server::TraceInfo &) {
  profile_.auth([&]() {
    VLOG(1)(R"(auth={})"_fmt, auth);
    LOG(INFO)("Ready"_sv);
    assert(ready_ == false);
    ready_ = true;
    handler_(*this);
  });
}

void WebSocket::operator()(
    const json::Currencies &currencies, const server::TraceInfo &trace_info) {
  profile_.currencies([&]() {
    VLOG(1)(R"(currencies={})"_fmt, currencies);
    handler_(currencies, trace_info);
  });
}

void WebSocket::operator()(
    const json::Instruments &instruments, const server::TraceInfo &trace_info) {
  profile_.instruments([&]() {
    VLOG(1)(R"(instruments={})"_fmt, instruments);
    handler_(instruments, trace_info);
  });
}

void WebSocket::operator()(const json::Positions &positions, const server::TraceInfo &trace_info) {
  profile_.positions([&]() {
    VLOG(1)(R"(positions={})"_fmt, positions);
    handler_(positions, trace_info);
  });
}

void WebSocket::operator()(const json::Ticker &ticker, const server::TraceInfo &trace_info) {
  profile_.ticker([&]() {
    VLOG(2)(R"(ticker={})"_fmt, ticker);
    handler_(ticker, trace_info);
  });
}

}  // namespace deribit
}  // namespace roq
