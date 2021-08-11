/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/web_socket.h"

#include <algorithm>

#include "roq/utils/compare.h"
#include "roq/utils/mask.h"
#include "roq/utils/update.h"

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
static const auto NAME = "ws"_sv;
static const auto SUPPORTS = utils::Mask{
    SupportType::TOP_OF_BOOK,
};
static const auto SUPPORTS_MASTER = utils::Mask{
    SUPPORTS,
    SupportType::MARKET_STATUS,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

WebSocket::WebSocket(
    Handler &handler, core::io::Context &context, uint16_t stream_id, Shared &shared, bool master)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"_sv, stream_id_, NAME)),
      master_(master), connection_(
                           *this,
                           context,
                           core::URI(Flags::ws_uri()),
                           {},  // query
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
          .quote = create_metrics(name_, "quote"_sv),
          .ticker = create_metrics(name_, "ticker"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
          .heartbeat = create_metrics(name_, "heartbeat"_sv),
      },
      shared_(shared),
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
      .write(profile_.quote, metrics::PROFILE)
      .write(profile_.ticker, metrics::PROFILE)
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void WebSocket::update_subscriptions(std::vector<std::string> &symbols) {
  assert(&symbols != &symbols_);
  auto max_size = Flags::ws_market_data_max_subscriptions_per_stream();
  auto offset = symbols_.size();
  if (max_size <= offset)
    return;
  if (symbols.empty())
    return;
  symbols_.reserve(max_size);
  auto length = std::min(max_size - offset, symbols.size());
  assert(length > 0);
  for (size_t i = {}; i < length; ++i) {
    symbols_.emplace_back(symbols.back());
    symbols.pop_back();
  }
  assert(length == (symbols_.size() - offset));
  if (ready_) {
    subscribe_quote({&symbols_[offset], length});
    subscribe_ticker({&symbols_[offset], length});
  }
}

void WebSocket::operator()(const core::web::Socket::Connected &) {
  // note! wait for upgrade
}

void WebSocket::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void WebSocket::operator()(const core::web::Socket::Ready &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void WebSocket::operator()(const core::web::Socket::Close &) {
}

void WebSocket::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(trace_info, external_latency, handler_);
  latency_.ping.update(latency.sample);
}

void WebSocket::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void WebSocket::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    server::TraceInfo trace_info;
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = (master_ ? SUPPORTS_MASTER : SUPPORTS).get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"_sv, stream_status);
    server::create_trace_and_dispatch(trace_info, stream_status, handler_);
  }
}

uint32_t WebSocket::download(WebSocketState state) {
  switch (state) {
    case WebSocketState::UNDEFINED:
      break;
    case WebSocketState::CURRENCIES:
      if (!master_)
        return {};
      return download_currencies();
    case WebSocketState::INSTRUMENTS:
      if (!master_)
        return {};
      return download_instruments();
    case WebSocketState::SUBSCRIBE:
      if (master_) {
        subscribe_platform_state();
        subscribe_instrument_state();
      }
      subscribe_quote(symbols_);
      subscribe_ticker(symbols_);
      return {};
    case WebSocketState::DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return {};
  }
  assert(false);
  return {};
}

uint32_t WebSocket::download_currencies() {
  get_currencies();
  return 1;
}

uint32_t WebSocket::download_instruments() {
  for (auto &currency : all_currencies_)
    get_instruments(currency);
  return all_currencies_.size();
}

void WebSocket::get_currencies() {
  constexpr json::RequestType request_type = json::RequestType::GET_CURRENCIES;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/get_currencies",)"
      R"("params":{{}},)"
      R"("id":"{}")"
      R"(}})"_sv,
      request_type.as_raw_text());
  connection_.send_text(message);
}

void WebSocket::get_instruments(const std::string_view &currency) {
  constexpr json::RequestType request_type = json::RequestType::GET_INSTRUMENTS;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/get_instruments",)"
      R"("params":{{)"
      R"("currency":"{}")"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_sv,
      currency,
      request_type.as_raw_text());
  connection_.send_text(message);
}

void WebSocket::subscribe_platform_state() {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_PLATFORM_STATE;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["platform_state"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_sv,
      request_type.as_raw_text());
  connection_.send_text(message);
}

void WebSocket::subscribe_instrument_state() {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_INSTRUMENT_STATE;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["instrument.state.any.any"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_sv,
      request_type.as_raw_text());
  connection_.send_text(message);
}

void WebSocket::subscribe_quote(const roq::span<std::string> &symbols) {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_QUOTE;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["quote.{}"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_sv,
      fmt::join(symbols, R"(","quote.)"_sv),
      request_type.as_raw_text());
  connection_.send_text(message);
}

void WebSocket::subscribe_ticker(const roq::span<std::string> &symbols) {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_TICKER;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["ticker.{}.raw"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_sv,
      fmt::join(symbols, R"(.raw","ticker.)"_sv),
      request_type.as_raw_text());
  connection_.send_text(message);
}

void WebSocket::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      core::jsonrpc::Parser::dispatch(*this, message);
    } catch (...) {
      log::warn(R"(message="{}")"_sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void WebSocket::operator()(const core::jsonrpc::Error &error, core::json::value_t &value) {
  json::Error error_2(value);
  log::fatal(R"(error={}, id="{}")"_sv, error_2, error.id);
}

void WebSocket::operator()(const core::jsonrpc::Result &result, core::json::value_t &value) {
  server::TraceInfo trace_info;  // XXX not correct (*parsing* has already started)
  json::RequestType request_type(result.id);
  switch (request_type) {
    case json::RequestType::UNDEFINED:
      break;
    case json::RequestType::UNKNOWN:
      log::fatal(R"(Unknown request_type="{}")"_sv, result.id);
      break;
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
    case json::RequestType::SUBSCRIBE_PLATFORM_STATE:
    case json::RequestType::SUBSCRIBE_INSTRUMENT_STATE:
    case json::RequestType::SUBSCRIBE_QUOTE:
    case json::RequestType::SUBSCRIBE_TICKER:
      break;
    default:
      log::fatal("Unexpected: request_type={}"_sv, request_type);
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
      log::fatal(R"(Unknown method="{}")"_sv, notification.method);
      break;
    case json::Method::SUBSCRIPTION: {
      core::json::Buffer buffer(decode_buffer_);
      json::Parser::dispatch(*this, value, buffer, trace_info);
      break;
    }
  }
}

void WebSocket::operator()(const json::Auth &, const server::TraceInfo &) {
  log::fatal("Unexpected"_sv);
}

void WebSocket::operator()(const json::Currencies &currencies, const server::TraceInfo &) {
  profile_.currencies([&]() {
    log::info<1>("currencies={}"_sv, currencies);
    auto &data = currencies.data;
    std::vector<std::string> currencies;
    if (!data.empty())
      currencies.reserve(data.size());
    for (auto &item : data) {
      auto &currency = item.currency;
      if (all_currencies_.emplace(currency).second)
        currencies.emplace_back(currency);
    }
    download_.check(WebSocketState::CURRENCIES);
    if (!currencies.empty()) {
      CurrenciesUpdate currencies_update{
          .currencies = currencies,
      };
      handler_(currencies_update);
    }
  });
}

void WebSocket::operator()(const json::Instruments &instruments, const server::TraceInfo &) {
  profile_.instruments([&]() {
    log::info<1>("instruments={}"_sv, instruments);
    auto &data = instruments.data;
    std::vector<std::string> symbols;
    if (!data.empty())
      symbols.reserve(data.size());
    for (auto &item : data) {
      auto &symbol = item.instrument_name;
      if (shared_.discard_symbol(symbol))
        continue;
      if (all_symbols_.emplace(symbol).second) {
        symbols.emplace_back(symbol);
        // cache multiplier so Quote (amount) can be converted to TopOfBook (lots)
        // note! the multiplier is only cached on startup!
        auto multiplier =
            utils::compare(item.contract_size, 0.0) == 0 ? 1.0 : (1.0 / item.contract_size);
        shared_.multiplier[symbol] = multiplier;
      }
    }
    download_.check(WebSocketState::INSTRUMENTS);
    if (!symbols.empty()) {
      SymbolsUpdate symbols_update{
          .symbols = symbols,
      };
      handler_(symbols_update);
    }
  });
}

void WebSocket::operator()(const json::Positions &, const server::TraceInfo &) {
  log::fatal("Unexpected"_sv);
}

void WebSocket::operator()(const server::Trace<json::PlatformState> &) {
}

void WebSocket::operator()(const server::Trace<json::InstrumentState> &) {
  // seldom updated -- also done by Ticker
}

void WebSocket::operator()(const server::Trace<json::Quote> &event) {
  profile_.quote([&]() {
    auto &trace_info = event.trace_info;
    auto &quote = event.value;
    log::info<2>("quote={}"_sv, quote);
    if (get_top_of_book(quote.instrument_name, [&](auto &layer, auto multiplier) {
          auto bid_quantity = multiplier * quote.best_bid_amount;
          auto ask_quantity = multiplier * quote.best_ask_amount;
          TopOfBook top_of_book = {
              .stream_id = stream_id_,
              .exchange = Flags::exchange(),
              .symbol = quote.instrument_name,
              .layer{
                  .bid_price = quote.best_bid_price,
                  .bid_quantity = bid_quantity,
                  .ask_price = quote.best_ask_price,
                  .ask_quantity = ask_quantity,
              },
              .snapshot = false,
              .exchange_time_utc = quote.timestamp,
          };
          if (utils::compare(layer, top_of_book.layer) != 0) {
            layer = top_of_book.layer;
            server::create_trace_and_dispatch(trace_info, top_of_book, handler_, true);
          }
        })) {
    } else {
      log::warn<3>(
          R"(Unexpected: can't find multiplier for symbol="{}")"_sv, quote.instrument_name);
    }
  });
}

void WebSocket::operator()(const server::Trace<json::Ticker> &event) {
  profile_.ticker([&]() {
    auto &trace_info = event.trace_info;
    auto &ticker = event.value;
    log::info<2>("ticker={}"_sv, ticker);
    auto trading_status = json::map(ticker.state);
    auto &item = trading_status_[ticker.instrument_name];
    if (trading_status && utils::update(item, trading_status)) {
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

void WebSocket::operator()(const server::Trace<json::Portfolio> &) {
  log::fatal("Unexpected"_sv);
}

void WebSocket::operator()(const server::Trace<json::Changes> &) {
  log::fatal("Unexpected"_sv);
}

void WebSocket::operator()(const server::Trace<json::Order> &) {
  log::fatal("Unexpected"_sv);
}

void WebSocket::operator()(const server::Trace<json::Trades2> &) {
  log::fatal("Unexpected"_sv);
}

}  // namespace deribit
}  // namespace roq
