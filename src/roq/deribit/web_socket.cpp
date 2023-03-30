/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/web_socket.hpp"

#include <algorithm>

#include "roq/mask.hpp"
#include "roq/utils/compare.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/socket/client_factory.hpp"

#include "roq/deribit/utils.hpp"

#include "roq/deribit/flags/common.hpp"
#include "roq/deribit/flags/config.hpp"
#include "roq/deribit/flags/multicast.hpp"
#include "roq/deribit/flags/web_socket.hpp"

#include "roq/deribit/json/error.hpp"
#include "roq/deribit/json/method.hpp"
#include "roq/deribit/json/request_type.hpp"
#include "roq/deribit/json/utils.hpp"

using namespace std::literals;

using namespace fmt::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const NAME = "ws"sv;
}

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"_cf, stream_id, NAME);
}

auto publish_top_of_book(auto const &shared) {
  return !shared.has_multicast() || flags::Multicast::multicast_disable_top_of_book();
}

auto get_supports(auto master, auto publish_top_of_book) {
  Mask<SupportType> result;
  if (master)
    result |= SupportType::MARKET_STATUS;
  if (publish_top_of_book)
    result |= SupportType::TOP_OF_BOOK;
  return result;
}

auto create_connection(auto &handler, auto &context) {
  auto uri = flags::WebSocket::ws_uri();
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .validate_certificate = server::Flags::net_tls_validate_certificate(),
      // connection manager
      .connection_timeout = server::Flags::net_connection_timeout(),
      .disconnect_on_idle_timeout = server::Flags::net_disconnect_on_idle_timeout(),
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = flags::WebSocket::ws_ping_freq(),
      // implementation
      .decode_buffer_size = flags::Common::decode_buffer_size(),
      .encode_buffer_size = flags::Common::encode_buffer_size(),
  };
  return web::socket::ClientFactory::create(handler, context, config, []() { return std::string(); });
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(auto const &group, auto const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

// === IMPLEMENTATION ===

WebSocket::WebSocket(
    Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index, bool master)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, master_{master},
      publish_top_of_book_{publish_top_of_book(shared)}, supports_{get_supports(master_, publish_top_of_book_)},
      connection_{create_connection(*this, context)}, decode_buffer_{flags::Common::decode_buffer_size()},
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
      shared_{shared},
      download_{flags::WebSocket::ws_request_timeout(), [this](auto state) { return download(state); }} {
  log::info("DEBUG: publish_top_of_book={}"sv, publish_top_of_book_);
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
  if ((*connection_).ready())
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

void WebSocket::operator()(web::socket::Client::Connected const &) {
  // note! wait for upgrade
}

void WebSocket::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
  subscribe_queue_.clear();
}

void WebSocket::operator()(web::socket::Client::Ready const &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
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
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

uint32_t WebSocket::download(WebSocketState state) {
  switch (state) {
    using enum WebSocketState;
    case UNDEFINED:
      break;
    case CURRENCIES:
      if (!master_)
        return {};
      return download_currencies();
    case INSTRUMENTS:
      if (!master_)
        return {};
      return download_instruments();
    case SUBSCRIBE:
      assert(!ready_);
      ready_ = true;
      if (master_) {
        subscribe_platform_state();
        subscribe_instrument_state();
      }
      subscribe();
      return {};
    case DONE:
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
  const json::RequestType request_type = json::RequestType::GET_CURRENCIES;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/get_currencies",)"
      R"("params":{{}},)"
      R"("id":"{}")"
      R"(}})"_cf,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::get_instruments(std::string_view const &currency) {
  const json::RequestType request_type = json::RequestType::GET_INSTRUMENTS;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/get_instruments",)"
      R"("params":{{)"
      R"("currency":"{}")"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_cf,
      currency,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::subscribe_platform_state() {
  const json::RequestType request_type = json::RequestType::SUBSCRIBE_PLATFORM_STATE;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["platform_state"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_cf,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::subscribe_instrument_state() {
  const json::RequestType request_type = json::RequestType::SUBSCRIBE_INSTRUMENT_STATE;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["instrument.state.any.any"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_cf,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols))
    return;
  if (!flags::WebSocket::ws_disable_quote())
    subscribe_quote(symbols);
  subscribe_ticker(symbols);
}

void WebSocket::subscribe_quote(std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  const json::RequestType request_type = json::RequestType::SUBSCRIBE_QUOTE;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["quote.{}"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_cf,
      fmt::join(symbols, R"(","quote.)"sv),
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::subscribe_ticker(std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  const json::RequestType request_type = json::RequestType::SUBSCRIBE_TICKER;
  auto interval = flags::WebSocket::ws_ticker_interval();
  auto separator = fmt::format(R"(.{}","ticker.)"sv, interval);
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/subscribe",)"
      R"("params":{{)"
      R"("channels":["ticker.{}.{}"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_cf,
      fmt::join(symbols, separator),
      interval,
      request_type.as_raw_text());
  // log::debug(R"(message="{}")"sv, message);
  subscribe_queue_.emplace_back(message);
}

void WebSocket::parse(std::string_view const &message) {
  profile_.parse([&]() {
    TraceInfo trace_info;
    try {
      core::jsonrpc::Parser::dispatch(*this, message, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void WebSocket::operator()(Trace<core::jsonrpc::Error> const &event, core::json::Value &value) {
  auto &[trace_info, error] = event;
  json::Error error_2{value};
  log::fatal(R"(error={}, id="{}")"sv, error_2, error.id);
}

void WebSocket::operator()(Trace<core::jsonrpc::Result> const &event, core::json::Value &value) {
  auto &[trace_info, result] = event;
  json::RequestType request_type(result.id);
  switch (request_type) {
    using enum json::RequestType::type_t;
    case UNDEFINED:
      break;
    case UNKNOWN:
      log::fatal(R"(Unknown request_type="{}")"sv, result.id);
      return;
    case AUTH:
      break;  // unexpected
    case GET_CURRENCIES: {
      core::json::Buffer buffer(decode_buffer_);
      const json::Currencies currencies{value, buffer};
      Trace event{trace_info, currencies};
      (*this)(event);
      return;
    }
    case GET_INSTRUMENTS: {
      core::json::Buffer buffer{decode_buffer_};
      const json::Instruments instruments{value, buffer};
      Trace event{trace_info, instruments};
      (*this)(event);
      return;
    }
    case SUBSCRIBE_PLATFORM_STATE:
    case SUBSCRIBE_INSTRUMENT_STATE:
    case SUBSCRIBE_QUOTE:
    case SUBSCRIBE_TICKER:
      // note! no need to parse
      return;
    case SUBSCRIBE_PORTFOLIO:
    case SUBSCRIBE_CHANGES:
    case SUBSCRIBE_ORDERS:
    case SUBSCRIBE_TRADES:
    case GET_ACCOUNT_SUMMARY:
    case GET_TRADES:
    case GET_POSITIONS:
      break;  // unexpected
  }
  log::fatal("Unexpected: request_type={}"sv, request_type);
}

void WebSocket::operator()(Trace<core::jsonrpc::Notification> const &event, core::json::Value &value) {
  auto &[trace_info, notification] = event;
  json::Method method{notification.method};
  switch (method) {
    using enum json::Method::type_t;
    case UNDEFINED:
      break;
    case UNKNOWN:
      log::fatal(R"(Unknown method="{}")"sv, notification.method);
      break;
    case SUBSCRIPTION: {
      core::json::Buffer buffer{decode_buffer_};
      json::Parser::dispatch(*this, value, buffer, trace_info);
      break;
    }
  }
}

void WebSocket::operator()(Trace<json::Auth> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::Currencies> const &event) {
  profile_.currencies([&]() {
    if (!master_)
      log::fatal("Unexpected"sv);
    auto &[trace_info, currencies] = event;
    log::info<2>("currencies={}"sv, currencies);
    (*connection_).touch(trace_info.source_receive_time);
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

void WebSocket::operator()(Trace<json::Instruments> const &event) {
  profile_.instruments([&]() {
    if (!master_)
      log::fatal("Unexpected"sv);
    auto &[trace_info, instruments] = event;
    (*connection_).touch(trace_info.source_receive_time);
    auto &data = instruments.data;
    std::vector<Symbol> symbols;
    if (!std::empty(data))
      symbols.reserve(std::size(data));
    for (auto &item : data) {
      log::info<2>("instrument={}"sv, item);
      auto &symbol = item.instrument_name;
      assert(!std::empty(symbol));
      auto discard = shared_.discard_symbol(symbol);
      // needed by multicast
      log::info<5>(R"(DEBUG: CREATE "{}" discard={})"sv, item.instrument_name, discard);
      auto multiplier = compute_contracts_multiplier(item.contract_size);
      shared_.instruments.try_emplace(
          item.instrument_id, Instrument{item.instrument_name, item.contract_size, multiplier}, discard);
      if (discard)
        continue;
      if (shared_.all_symbols.emplace(symbol).second)
        symbols.emplace_back(symbol);
      // cache multiplier so Quote (amount) can be converted to TopOfBook (lots)
      // note! the multiplier is only cached on startup!
      shared_.multiplier[symbol] = multiplier;
    }
    download_.check(WebSocketState::INSTRUMENTS);
    if (!std::empty(symbols)) {
      auto symbols_update = SymbolsUpdate{
          .symbols = symbols,
      };
      handler_(symbols_update);
    }
  });
}

void WebSocket::operator()(Trace<json::Positions> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::PlatformState> const &) {
  if (!master_)
    log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::InstrumentState> const &) {
  if (!master_)
    log::fatal("Unexpected"sv);
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
                .exchange = flags::Config::exchange(),
                .symbol = quote.instrument_name,
                .layer{
                    .bid_price = quote.best_bid_price,
                    .bid_quantity = bid_quantity,
                    .ask_price = quote.best_ask_price,
                    .ask_quantity = ask_quantity,
                },
                .update_type = UpdateType::INCREMENTAL,
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
    auto trading_status = json::map(ticker.state);
    auto &item = trading_status_[ticker.instrument_name];
    if (trading_status != TradingStatus{} && utils::update(item, trading_status)) {
      auto market_status = MarketStatus{
          .stream_id = stream_id_,
          .exchange = flags::Config::exchange(),
          .symbol = ticker.instrument_name,
          .trading_status = trading_status,
      };
      create_trace_and_dispatch(handler_, trace_info, market_status, true);
    }
  });
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

void WebSocket::check_subscribe_queue(std::chrono::nanoseconds now) {
  subscribe_queue_.dispatch(
      [&](auto now) { return shared_.rate_limiter.can_request(now); },
      [&](auto &message) {
        // log::debug(R"(Subscribe: "{}")"sv, message);
        (*connection_).send_text(message);
      },
      now);
}

}  // namespace deribit
}  // namespace roq
