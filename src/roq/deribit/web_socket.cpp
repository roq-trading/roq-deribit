/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/web_socket.h"

#include "roq/core/compare.h"
#include "roq/core/update.h"

#include "roq/core/metrics/factory.h"

#include "roq/deribit/flags.h"

#include "roq/deribit/json/error.h"
#include "roq/deribit/json/method.h"
#include "roq/deribit/json/request_type.h"
#include "roq/deribit/json/utils.h"

using namespace roq::literals;

namespace roq {
namespace deribit {

namespace {
static const auto CONNECTION = "ws"_sv;

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(Flags::name(), group, function) {}
};
}  // namespace

WebSocket::WebSocket(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared)
    : handler_(handler), stream_id_(stream_id),
      name_(roq::format("{}_{}"_fmt, CONNECTION, stream_id_)),
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
          .disconnect = create_metrics(name_, "disconnect"_sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"_sv),
          .auth = create_metrics(name_, "auth"_sv),
          .currencies = create_metrics(name_, "currencies"_sv),
          .instruments = create_metrics(name_, "instruments"_sv),
          .positions = create_metrics(name_, "positions"_sv),
          .ticker = create_metrics(name_, "ticker"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
          .heartbeat = create_metrics(name_, "heartbeat"_sv),
      },
      security_(security), shared_(shared),
      download_(Flags::ws_request_timeout(), [this](auto state) { return download(state); }) {
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
  (*this)(GatewayStatus::DISCONNECTED);
  download_.reset();
  currencies_.clear();
  symbols_.clear();
}

void WebSocket::operator()(const core::web::Socket::Ready &) {
  login();
  (*this)(GatewayStatus::LOGIN_SENT);
}

void WebSocket::operator()(const core::web::Socket::Close &) {
}

void WebSocket::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .name = name_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(trace_info, external_latency, handler_);
  latency_.ping.update(latency.sample);
}

void WebSocket::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void WebSocket::operator()(const GatewayStatus status) {
  if (core::update(status_, status)) {
    server::TraceInfo trace_info;
    MarketDataStatus market_data_status{
        .stream_id = stream_id_,
        .status = status_,
    };
    LOG(INFO)("market_data_status={}"_fmt, market_data_status);
    server::create_trace_and_dispatch(trace_info, market_data_status, handler_);
  }
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

uint32_t WebSocket::download(WebSocketState state) {
  switch (state) {
    case WebSocketState::UNDEFINED:
      break;
    case WebSocketState::CURRENCIES:
      return download_currencies();
    case WebSocketState::INSTRUMENTS:
      return download_instruments();
    case WebSocketState::POSITIONS:
      return download_positions();
    case WebSocketState::TICKERS:
      return download_tickers();
    case WebSocketState::DONE:
      (*this)(GatewayStatus::READY);
      assert(!ready_);
      ready_ = true;
      return 0u;
  }
  assert(false);
  return 0u;
}

uint32_t WebSocket::download_currencies() {
  assert(currencies_.empty());
  get_currencies();
  return 1u;
}

uint32_t WebSocket::download_instruments() {
  assert(symbols_.empty());
  for (auto &currency : currencies_)
    get_instruments(currency);
  return currencies_.size();
}

uint32_t WebSocket::download_positions() {
  for (auto &currency : currencies_)
    get_positions(currency);
  return currencies_.size();
}

uint32_t WebSocket::download_tickers() {
  subscribe_ticker(symbols_);
  return 0u;
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

void WebSocket::subscribe_ticker(const roq::span<std::string> &symbols) {
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

void WebSocket::unsubscribe_ticker(const roq::span<std::string> &symbols) {
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
    (*this)(GatewayStatus::DOWNLOADING);
    download_.begin();
  });
}

void WebSocket::operator()(const json::Currencies &currencies, const server::TraceInfo &) {
  profile_.currencies([&]() {
    VLOG(1)(R"(currencies={})"_fmt, currencies);
    assert(currencies_.empty());
    std::transform(
        currencies.data.begin(),
        currencies.data.end(),
        std::back_inserter(currencies_),
        [](const auto &item) { return std::string(item.currency); });
    download_.check(WebSocketState::CURRENCIES);
  });
}

void WebSocket::operator()(const json::Instruments &instruments, const server::TraceInfo &) {
  profile_.instruments([&]() {
    VLOG(1)(R"(instruments={})"_fmt, instruments);
    for (auto &item : instruments.data) {
      if (shared_.discard_symbol(item.instrument_name))
        continue;
      symbols_.emplace_back(item.instrument_name);
    }
    download_.check(WebSocketState::INSTRUMENTS);
  });
}

void WebSocket::operator()(const json::Positions &positions, const server::TraceInfo &) {
  profile_.positions([&]() {
    VLOG(1)(R"(positions={})"_fmt, positions);
    // XXX do something
    download_.check(WebSocketState::POSITIONS);
  });
}

void WebSocket::operator()(const json::Ticker &ticker, const server::TraceInfo &trace_info) {
  profile_.ticker([&]() {
    VLOG(2)(R"(ticker={})"_fmt, ticker);
    auto snapshot = status_ != GatewayStatus::READY;
    auto &layer = top_of_book_[ticker.instrument_name];
    TopOfBook top_of_book = {
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = ticker.instrument_name,
        .layer{
            .bid_price = ticker.best_bid_price,
            .bid_quantity = ticker.best_bid_amount,
            .ask_price = ticker.best_ask_price,
            .ask_quantity = ticker.best_ask_amount,
        },
        .snapshot = snapshot,
        .exchange_time_utc = ticker.timestamp,
    };
    if (core::compare(layer, top_of_book.layer) != 0) {
      layer = top_of_book.layer;
      server::create_trace_and_dispatch(trace_info, top_of_book, handler_, true);
    }
    auto trading_status = json::map(ticker.state);
    auto &item = trading_status_[ticker.instrument_name];
    if (core::update(item, trading_status)) {
      MarketStatus market_status{
          .stream_id = stream_id_,
          .exchange = Flags::exchange(),
          .symbol = ticker.instrument_name,
          .trading_status = trading_status,
      };
      server::create_trace_and_dispatch(trace_info, market_status, handler_, true);
    }
  });
}

}  // namespace deribit
}  // namespace roq
