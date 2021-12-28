/* Copyright (c) 2017-2022, Hans Erik Thrane */

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

using namespace std::literals;

namespace roq {
namespace deribit {

namespace {
static const auto NAME = "ws"sv;
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
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Shared &shared,
    size_t index,
    bool master)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      index_(index), master_(master), connection_(
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
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
          .auth = create_metrics(name_, "auth"sv),
          .currencies = create_metrics(name_, "currencies"sv),
          .instruments = create_metrics(name_, "instruments"sv),
          .quote = create_metrics(name_, "quote"sv),
          .ticker = create_metrics(name_, "ticker"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
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
  auto now = event.value.now;
  connection_.refresh(now);
  if (connection_.ready())
    check_subscribe_queue(now);
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

void WebSocket::subscribe(size_t start_from) {
  if (ready())
    subscribe(shared_.symbols.get_slice(index_, start_from));
}

void WebSocket::operator()(const core::web::ClientSocket::Connected &) {
  // note! wait for upgrade
}

void WebSocket::operator()(const core::web::ClientSocket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
  subscribe_queue_.clear();
}

void WebSocket::operator()(const core::web::ClientSocket::Ready &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void WebSocket::operator()(const core::web::ClientSocket::Close &) {
}

void WebSocket::operator()(const core::web::ClientSocket::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void WebSocket::operator()(const core::web::ClientSocket::Text &text) {
  parse(text.payload);
}

void WebSocket::operator()(const core::web::ClientSocket::Binary &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = (master_ ? SUPPORTS_MASTER : SUPPORTS).get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
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
      assert(!ready_);
      ready_ = true;
      if (master_) {
        subscribe_platform_state();
        subscribe_instrument_state();
      }
      subscribe();
      return {};
    case WebSocketState::DONE:
      (*this)(ConnectionStatus::READY);
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
  for (auto &currency : shared_.all_currencies)
    get_instruments(currency);
  return std::size(shared_.all_currencies);
}

void WebSocket::get_currencies() {
  constexpr json::RequestType request_type = json::RequestType::GET_CURRENCIES;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/get_currencies",)"
      R"("params":{{}},)"
      R"("id":"{}")"
      R"(}})"sv,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
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
      R"(}})"sv,
      currency,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
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
      R"(}})"sv,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
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
      R"(}})"sv,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::subscribe(const roq::span<std::string const> &symbols) {
  subscribe_quote(symbols);
  subscribe_ticker(symbols);
}

void WebSocket::subscribe_quote(const roq::span<std::string const> &symbols) {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_QUOTE;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["quote.{}"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"sv,
      fmt::join(symbols, R"(","quote.)"sv),
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::subscribe_ticker(const roq::span<std::string const> &symbols) {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_TICKER;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["ticker.{}.raw"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"sv,
      fmt::join(symbols, R"(.raw","ticker.)"sv),
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::parse(const std::string_view &message) {
  profile_.parse([&]() {
    auto trace_info = server::create_trace_info();
    try {
      core::jsonrpc::Parser::dispatch(*this, message, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void WebSocket::operator()(
    const server::Trace<core::jsonrpc::Error> &event, core::json::value_t &value) {
  auto &[trace_info, error] = event;
  json::Error error_2(value);
  log::fatal(R"(error={}, id="{}")"sv, error_2, error.id);
}

void WebSocket::operator()(
    const server::Trace<core::jsonrpc::Result> &event, core::json::value_t &value) {
  auto &[trace_info, result] = event;
  json::RequestType request_type(result.id);
  switch (request_type) {
    case json::RequestType::UNDEFINED:
      break;
    case json::RequestType::UNKNOWN:
      log::fatal(R"(Unknown request_type="{}")"sv, result.id);
      break;
    case json::RequestType::GET_CURRENCIES: {
      core::json::Buffer buffer(decode_buffer_);
      json::Currencies currencies(value, buffer);
      server::Trace event(trace_info, currencies);
      (*this)(event);
      break;
    }
    case json::RequestType::GET_INSTRUMENTS: {
      core::json::Buffer buffer(decode_buffer_);
      json::Instruments instruments(value, buffer);
      server::Trace event(trace_info, instruments);
      (*this)(event);
      break;
    }
    case json::RequestType::SUBSCRIBE_PLATFORM_STATE:
    case json::RequestType::SUBSCRIBE_INSTRUMENT_STATE:
    case json::RequestType::SUBSCRIBE_QUOTE:
    case json::RequestType::SUBSCRIBE_TICKER:
      break;
    default:
      log::fatal("Unexpected: request_type={}"sv, request_type);
  }
}

void WebSocket::operator()(
    const server::Trace<core::jsonrpc::Notification> &event, core::json::value_t &value) {
  auto &[trace_info, notification] = event;
  json::Method method(notification.method);
  switch (method) {
    case json::Method::UNDEFINED:
      break;
    case json::Method::UNKNOWN:
      log::fatal(R"(Unknown method="{}")"sv, notification.method);
      break;
    case json::Method::SUBSCRIPTION: {
      core::json::Buffer buffer(decode_buffer_);
      json::Parser::dispatch(*this, value, buffer, trace_info);
      break;
    }
  }
}

void WebSocket::operator()(const server::Trace<json::Auth> &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(const server::Trace<json::Currencies> &event) {
  profile_.currencies([&]() {
    auto &[trace_info, currencies] = event;
    log::info<2>("currencies={}"sv, currencies);
    auto &data = currencies.data;
    std::vector<std::string> tmp;
    if (!std::empty(data))
      tmp.reserve(std::size(data));
    for (auto &item : data) {
      auto &currency = item.currency;
      if (shared_.all_currencies.emplace(currency).second)
        tmp.emplace_back(currency);
    }
    download_.check(WebSocketState::CURRENCIES);
    if (!std::empty(tmp)) {
      CurrenciesUpdate currencies_update{
          .currencies = tmp,
      };
      handler_(currencies_update);
    }
  });
}

void WebSocket::operator()(const server::Trace<json::Instruments> &event) {
  profile_.instruments([&]() {
    auto &[trace_info, instruments] = event;
    log::info<2>("instruments={}"sv, instruments);
    auto &data = instruments.data;
    std::vector<std::string> symbols;
    if (!std::empty(data))
      symbols.reserve(std::size(data));
    for (auto &item : data) {
      auto &symbol = item.instrument_name;
      if (shared_.discard_symbol(symbol))
        continue;
      if (shared_.all_symbols.emplace(symbol).second) {
        symbols.emplace_back(symbol);
        // cache multiplier so Quote (amount) can be converted to TopOfBook (lots)
        // note! the multiplier is only cached on startup!
        auto multiplier =
            utils::compare(item.contract_size, 0.0) == 0 ? 1.0 : (1.0 / item.contract_size);
        shared_.multiplier[symbol] = multiplier;
      }
    }
    download_.check(WebSocketState::INSTRUMENTS);
    if (!std::empty(symbols)) {
      SymbolsUpdate symbols_update{
          .symbols = symbols,
      };
      handler_(symbols_update);
    }
  });
}

void WebSocket::operator()(const server::Trace<json::Positions> &) {
  log::fatal("Unexpected"sv);
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
    log::info<3>("quote={}"sv, quote);
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
              .update_type = UpdateType::INCREMENTAL,
              .exchange_time_utc = quote.timestamp,
          };
          if (utils::compare(layer, top_of_book.layer) != 0) {
            layer = top_of_book.layer;
            server::create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
          }
        })) {
    } else {
      log::warn<3>(R"(Unexpected: can't find multiplier for symbol="{}")"sv, quote.instrument_name);
    }
  });
}

void WebSocket::operator()(const server::Trace<json::Ticker> &event) {
  profile_.ticker([&]() {
    auto &trace_info = event.trace_info;
    auto &ticker = event.value;
    log::info<3>("ticker={}"sv, ticker);
    auto trading_status = json::map(ticker.state);
    auto &item = trading_status_[ticker.instrument_name];
    if (trading_status && utils::update(item, trading_status)) {
      MarketStatus market_status{
          .stream_id = stream_id_,
          .exchange = Flags::exchange(),
          .symbol = ticker.instrument_name,
          .trading_status = trading_status,
      };
      server::create_trace_and_dispatch(handler_, trace_info, market_status, true);
    }
  });
}

void WebSocket::operator()(const server::Trace<json::Portfolio> &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(const server::Trace<json::Changes> &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(const server::Trace<json::Order> &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(const server::Trace<json::Trades2> &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::check_subscribe_queue(std::chrono::nanoseconds now) {
  subscribe_queue_.dispatch(
      [&](auto now) { return shared_.rate_limiter.can_request(now); },
      [&](auto &message) {
        log::debug(R"(Subscribe: "{}")"sv, message);
        connection_.send_text(message);
      },
      now);
}

}  // namespace deribit
}  // namespace roq
