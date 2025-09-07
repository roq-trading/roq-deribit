/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/deribit/web_socket.hpp"

#include <algorithm>

#include "roq/mask.hpp"

#include "roq/utils/compare.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/deribit/utils.hpp"

#include "roq/deribit/json/error.hpp"
#include "roq/deribit/json/map.hpp"
#include "roq/deribit/json/method.hpp"
#include "roq/deribit/json/request_type.hpp"
#include "roq/deribit/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const NAME = "ws"sv;

size_t const MAX_DECODE_BUFFER_DEPTH = 1;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto publish_top_of_book(auto &shared) {
  return !shared.has_multicast() || shared.settings.multicast.disable_top_of_book;
}

auto get_supports(auto master, auto publish_top_of_book) {
  Mask<SupportType> result;
  if (master) {
    result |= SupportType::MARKET_STATUS;
  }
  if (publish_top_of_book) {
    result |= SupportType::TOP_OF_BOOK;
  }
  return result;
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.ws.uri;
  auto config = web::socket::Client::Config{
      // connection
      .interface = settings.misc.test_local_interface,
      .uris = {&uri, 1},
      .host = settings.ws.host,
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = settings.net.disconnect_on_idle_timeout,
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.ping_freq,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::socket::Client::create(handler, context, config, []() { return std::string(); });
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

WebSocket::WebSocket(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared, Request &request, size_t index, bool master)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, master_{master},
      publish_top_of_book_{publish_top_of_book(shared)}, supports_{get_supports(master_, publish_top_of_book_)},
      connection_{create_connection(*this, shared.settings, context)}, decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .auth = create_metrics(shared.settings, name_, "auth"sv),
          .currencies = create_metrics(shared.settings, name_, "currencies"sv),
          .instruments = create_metrics(shared.settings, name_, "instruments"sv),
          .quote = create_metrics(shared.settings, name_, "quote"sv),
          .ticker = create_metrics(shared.settings, name_, "ticker"sv),
          .chart_trades = create_metrics(shared.settings, name_, "chart_trades"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .heartbeat = create_metrics(shared.settings, name_, "heartbeat"sv),
      },
      account_{account}, shared_{shared}, download_{shared.settings.ws.request_timeout, [this](auto state) { return download(state); }}, request_{request} {
}

void WebSocket::operator()(Event<Start> const &) {
  (*connection_).start();
}

void WebSocket::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void WebSocket::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
  if ((*connection_).ready()) {
    check_subscribe_queue(now);
  }
  check_currencies();
  check_instruments();
}

void WebSocket::operator()(metrics::Writer &writer) const {
  writer  //
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.auth, metrics::Type::PROFILE)
      .write(profile_.currencies, metrics::Type::PROFILE)
      .write(profile_.instruments, metrics::Type::PROFILE)
      .write(profile_.quote, metrics::Type::PROFILE)
      .write(profile_.ticker, metrics::Type::PROFILE)
      .write(profile_.chart_trades, metrics::Type::PROFILE)
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

void WebSocket::subscribe(size_t start_from) {
  if (ready()) {
    subscribe(shared_.symbols.get_slice(index_, start_from));
  }
}

void WebSocket::operator()(web::socket::Client::Connected const &) {
  // note! wait for upgrade
}

void WebSocket::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
  subscribe_queue_.clear();
  request_.request_instruments = {};
}

void WebSocket::operator()(web::socket::Client::Ready const &) {
  login();
  (*this)(ConnectionStatus::LOGIN_SENT);
}

void WebSocket::operator()(web::socket::Client::Close const &) {
}

void WebSocket::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void WebSocket::operator()(web::socket::Client::Text const &text) {
  parse(text.payload);
}

void WebSocket::operator()(web::socket::Client::Binary const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
        .supports = supports_,
        .transport = Transport::TCP,
        .protocol = Protocol::WS,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
        .interface = (*connection_).get_interface(),
        .authority = (*connection_).get_current_authority(),
        .path = (*connection_).get_current_path(),
        .proxy = (*connection_).get_proxy(),
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void WebSocket::login() {
  constexpr json::RequestType request_type = json::RequestType::AUTH;
  auto now = clock::get_realtime<std::chrono::milliseconds>();
  auto nonce = account_.create_nonce();
  auto [signature, timestamp] = account_.create_signature(now, nonce);
  auto message = fmt::format(
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
      R"(}})"sv,
      account_.key,
      timestamp.count(),
      nonce,
      signature,
      request_type.as_raw_text());
  (*connection_).send_text(message);
}

uint32_t WebSocket::download(WebSocketState state) {
  switch (state) {
    using enum WebSocketState;
    case UNDEFINED:
      break;
    case CURRENCIES:
      if (!master_) {
        return 0;
      }
      download_currencies();
      return 1;
    case INSTRUMENTS: {
      if (!master_) {
        return 0;
      }
      download_instruments();
      return 1;
    }
    case SUBSCRIBE:
      assert(!ready_);
      ready_ = true;
      if (master_) {
        subscribe_platform_state();
        subscribe_instrument_state();
      }
      subscribe();
      return 0;
    case DONE:
      handler_(Latch{});
      (*this)(ConnectionStatus::READY);
      return 0;
  }
  assert(false);
  return 0;
}

void WebSocket::download_currencies() {
  request_.request_currencies = clock::get_system();
}

void WebSocket::check_currencies() {
  if (download_.state() != WebSocketState::CURRENCIES) {
    return;
  }
  if (request_.request_currencies < request_.respond_currencies) {
    log::info("Currencies download has completed!"sv);
    download_.check(WebSocketState::CURRENCIES);
  }
}

void WebSocket::download_instruments() {
  request_.request_instruments = clock::get_system();
}

void WebSocket::check_instruments() {
  if (download_.state() != WebSocketState::INSTRUMENTS) {
    return;
  }
  if (request_.request_instruments < request_.respond_instruments) {
    log::info("Instruments download has completed!"sv);
    download_.check(WebSocketState::INSTRUMENTS);
  }
}

void WebSocket::subscribe_platform_state() {
  json::RequestType const request_type = json::RequestType::SUBSCRIBE_PLATFORM_STATE;
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
  json::RequestType const request_type = json::RequestType::SUBSCRIBE_INSTRUMENT_STATE;
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

void WebSocket::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols)) {
    return;
  }
  if (!shared_.settings.ws.disable_quote) {
    subscribe_quote(symbols);
  }
  subscribe_ticker(symbols);
  if (shared_.settings.download.time_series && shared_.settings.time_series.lookback.count()) {
    subscribe_chart_trades(symbols);
  }
}

void WebSocket::subscribe_quote(std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  json::RequestType const request_type = json::RequestType::SUBSCRIBE_QUOTE;
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

void WebSocket::subscribe_ticker(std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  json::RequestType const request_type = json::RequestType::SUBSCRIBE_TICKER;
  auto interval = shared_.settings.ws.ticker_interval;
  auto separator = fmt::format(R"(.{}","ticker.)"sv, interval);
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["ticker.{}.{}"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"sv,
      fmt::join(symbols, separator),
      interval,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::subscribe_chart_trades(std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  json::RequestType const request_type = json::RequestType::SUBSCRIBE_CHART_TRADES;
  auto interval = 1;  // 1 min
  auto separator = fmt::format(R"(.{}","chart.trades.)"sv, interval);
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["chart.trades.{}.{}"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"sv,
      fmt::join(symbols, separator),
      interval,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
  for (auto &symbol : symbols) {
    shared_.time_series_request_queue.emplace_back(symbol);
  }
}

void WebSocket::parse(std::string_view const &message) {
  profile_.parse([&]() {
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    TraceInfo trace_info;
    try {
      if (!core::jsonrpc::Parser::dispatch(*this, message, trace_info)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

void WebSocket::operator()(Trace<core::jsonrpc::Error> const &event, core::json::Value &value) {
  auto &[trace_info, error] = event;
  json::Error error_2{value};
  log::fatal(R"(error={}, id="{}")"sv, error_2, error.id);
}

bool WebSocket::operator()(Trace<core::jsonrpc::Result> const &event, core::json::Value &value) {
  auto &[trace_info, result] = event;
  json::RequestType request_type{result.id};
  switch (request_type) {
    using enum json::RequestType::type_t;
    case UNDEFINED_INTERNAL:
      break;
    case UNKNOWN_INTERNAL:
      log::warn(R"(Unknown request_type="{}")"sv, result.id);
      break;
    case AUTH: {
      json::Auth auth{value};
      Trace event{trace_info, auth};
      (*this)(event);
      return true;
    }
    case SUBSCRIBE_PLATFORM_STATE:
    case SUBSCRIBE_INSTRUMENT_STATE:
    case SUBSCRIBE_QUOTE:
    case SUBSCRIBE_TICKER:
    case SUBSCRIBE_CHART_TRADES:
      return true;  // note! no need to parse
    case SUBSCRIBE_PORTFOLIO:
    case SUBSCRIBE_CHANGES:
    case SUBSCRIBE_ORDERS:
    case SUBSCRIBE_TRADES:
    case GET_ACCOUNT_SUMMARY:
    case GET_TRADES:
    case GET_POSITIONS:
      break;  // unexpected
  }
  return false;
}

bool WebSocket::operator()(Trace<core::jsonrpc::Notification> const &event, core::json::Value &value) {
  auto &[trace_info, notification] = event;
  json::Method method{notification.method};
  switch (method) {
    using enum json::Method::type_t;
    case UNDEFINED_INTERNAL:
      break;
    case UNKNOWN_INTERNAL:
      log::warn(R"(Unknown method="{}")"sv, notification.method);
      break;
    case SUBSCRIPTION:
      return json::Parser::dispatch(*this, value, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types);
  }
  return false;
}

void WebSocket::operator()(Trace<json::Auth> const &event) {
  profile_.auth([&]() {
    auto &[trace_info, auth] = event;
    log::info<2>("auth={}"sv, auth);
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  });
}

void WebSocket::operator()(Trace<json::Positions> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::PlatformState> const &) {
  if (!master_) {
    log::fatal("Unexpected"sv);
  }
}

void WebSocket::operator()(Trace<json::InstrumentState> const &) {
  if (!master_) {
    log::fatal("Unexpected"sv);
  }
  // seldom updated -- also done by Ticker
}

void WebSocket::operator()(Trace<json::Quote> const &event) {
  profile_.quote([&]() {
    auto &trace_info = event.trace_info;
    auto &quote = event.value;
    log::info<3>("quote={}"sv, quote);
    (*connection_).touch(trace_info.source_receive_time);
    if (publish_top_of_book_) {
      if (get_top_of_book(quote.instrument_name, [&](auto &layer, auto multiplier) {
            // note! as real amounts to match MbP
            auto bid_quantity = multiplier * quote.best_bid_amount;
            auto ask_quantity = multiplier * quote.best_ask_amount;
            auto top_of_book = TopOfBook{
                .stream_id = stream_id_,
                .exchange = shared_.settings.exchange,
                .symbol = quote.instrument_name,
                .layer{
                    .bid_price = quote.best_bid_price,
                    .bid_quantity = bid_quantity,
                    .ask_price = quote.best_ask_price,
                    .ask_quantity = ask_quantity,
                },
                .update_type = UpdateType::SNAPSHOT,
                .exchange_time_utc = quote.timestamp,  // XXX not sure
                .exchange_sequence = {},
                .sending_time_utc = {},
            };
            if (!utils::is_equal(layer, top_of_book.layer)) {
              layer = top_of_book.layer;
              create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
            }
          })) {
      } else {
        log::warn<3>(R"(Unexpected: can't find multiplier for symbol="{}")"sv, quote.instrument_name);
      }
    }
  });
}

void WebSocket::operator()(Trace<json::Ticker> const &event) {
  profile_.ticker([&]() {
    auto &[trace_info, ticker] = event;
    log::info<3>("ticker={}"sv, ticker);
    (*connection_).touch(trace_info.source_receive_time);
    auto trading_status = map(ticker.state).template get<TradingStatus>();
    auto &item = trading_status_[ticker.instrument_name];
    if (trading_status != TradingStatus{} && utils::update(item, trading_status)) {
      auto market_status = MarketStatus{
          .stream_id = stream_id_,
          .exchange = shared_.settings.exchange,
          .symbol = ticker.instrument_name,
          .trading_status = trading_status,
      };
      create_trace_and_dispatch(handler_, trace_info, market_status, true);
    }
  });
}

void WebSocket::operator()(Trace<json::ChartTrades> const &event, std::string_view const &symbol, uint32_t interval) {
  auto &[trace_info, chart_trades] = event;
  log::debug(R"(chart_trades={}, symbol="{}", interval={})"sv, chart_trades, symbol, interval);
}

void WebSocket::operator()(Trace<json::Portfolio> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::Changes> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::Order> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::Trades2> const &) {
  log::fatal("Unexpected"sv);
}

// request

void WebSocket::check_subscribe_queue(std::chrono::nanoseconds now) {
  subscribe_queue_.dispatch([&](auto now) { return shared_.rate_limiter.can_request(now); }, [&](auto &message) { (*connection_).send_text(message); }, now);
}

}  // namespace deribit
}  // namespace roq
