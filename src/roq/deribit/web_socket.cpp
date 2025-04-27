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
}

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
  if (master)
    result |= SupportType::MARKET_STATUS;
  if (publish_top_of_book)
    result |= SupportType::TOP_OF_BOOK;
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

auto to_security_type(auto kind, [[maybe_unused]] auto instrument_type, auto settlement_period) -> SecurityType {
  switch (kind) {
    using enum json::Kind::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      return {};
    case FUTURE:
      /*
      switch (instrument_type) {
        using enum json::InstrumentType::type_t;
        case UNDEFINED__:
        case UNKNOWN__:
          return SecurityType::FUTURES;
        case LINEAR:
        case REVERSED:
          return SecurityType::SWAP;
      }
      */
      switch (settlement_period) {
        using enum json::SettlementPeriod::type_t;
        case UNDEFINED__:
        case UNKNOWN__:
          return SecurityType::FUTURES;
        case PERPETUAL:
          return SecurityType::SWAP;
        case DAY:
        case WEEK:
        case MONTH:
          return SecurityType::FUTURES;
      }
      break;
    case OPTION:
      return SecurityType::OPTION;
    case FUTURE_COMBO:
      return SecurityType::FUTURES;  // ???
    case OPTION_COMBO:
      return SecurityType::OPTION;  // ???
    case SPOT:
      return SecurityType::SPOT;
  }
  log::fatal("Unexpected"sv);
}

auto to_option_type(auto option_type) -> OptionType {
  switch (option_type) {
    using enum json::OptionType::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      return {};
    case CALL:
      return OptionType::CALL;
    case PUT:
      return OptionType::PUT;
  }
  log::fatal("Unexpected"sv);
}
}  // namespace

// === IMPLEMENTATION ===

WebSocket::WebSocket(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared, size_t index, bool master)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, master_{master},
      publish_top_of_book_{publish_top_of_book(shared)}, supports_{get_supports(master_, publish_top_of_book_)},
      connection_{create_connection(*this, shared.settings, context)}, decode_buffer_(shared.settings.misc.decode_buffer_size),
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
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .heartbeat = create_metrics(shared.settings, name_, "heartbeat"sv),
      },
      account_{account}, shared_{shared}, download_{shared.settings.ws.request_timeout, [this](auto state) { return download(state); }} {
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
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.auth, metrics::Type::PROFILE)
      .write(profile_.currencies, metrics::Type::PROFILE)
      .write(profile_.instruments, metrics::Type::PROFILE)
      .write(profile_.quote, metrics::Type::PROFILE)
      .write(profile_.ticker, metrics::Type::PROFILE)
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
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
      if (!master_)
        return 0;
      return download_currencies();
    case INSTRUMENTS:
      if (!master_)
        return 0;
      return download_instruments();
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
  json::RequestType const request_type = json::RequestType::GET_CURRENCIES;
  auto message = fmt::format(
      R"({{)"
      R"("method":"public/get_currencies",)"
      R"("params":{{}},)"
      R"("id":"{}")"
      R"(}})"sv,
      request_type.as_raw_text());
  subscribe_queue_.emplace_back(message);
}

void WebSocket::get_instruments(std::string_view const &currency) {
  json::RequestType const request_type = json::RequestType::GET_INSTRUMENTS;
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
  if (std::empty(symbols))
    return;
  if (!shared_.settings.ws.disable_quote)
    subscribe_quote(symbols);
  subscribe_ticker(symbols);
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

void WebSocket::parse(std::string_view const &message) {
  profile_.parse([&]() {
    auto log_message = [&]() { log::warn(R"(message="{}")"sv, message); };
    TraceInfo trace_info;
    try {
      if (!core::jsonrpc::Parser::dispatch(*this, message, trace_info))
        log_message();
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
    case UNDEFINED__:
      break;
    case UNKNOWN__:
      log::warn(R"(Unknown request_type="{}")"sv, result.id);
      break;
    case AUTH: {
      json::Auth auth{value};
      Trace event{trace_info, auth};
      (*this)(event);
      return true;
    }
    case GET_CURRENCIES: {
      core::json::Buffer buffer{decode_buffer_};
      json::Currencies currencies{value, buffer};
      Trace event{trace_info, currencies};
      (*this)(event);
      return true;
    }
    case GET_INSTRUMENTS: {
      profile_.instruments([&]() {
        std::vector<Symbol> symbols;
        for (auto item : std::get<core::json::Array>(value)) {
          core::json::Buffer buffer{decode_buffer_};
          json::Instrument instrument{item, buffer};
          Trace event{trace_info, instrument};
          auto discard = (*this)(event);
          if (!discard && shared_.all_symbols.emplace(instrument.instrument_name).second) {
            symbols.emplace_back(instrument.instrument_name);
          }
        }
        download_.check(WebSocketState::INSTRUMENTS);
        if (!std::empty(symbols)) {
          auto symbols_update = SymbolsUpdate{
              .symbols = symbols,
          };
          handler_(symbols_update);
        }
      });
      return true;
    }
    case SUBSCRIBE_PLATFORM_STATE:
    case SUBSCRIBE_INSTRUMENT_STATE:
    case SUBSCRIBE_QUOTE:
    case SUBSCRIBE_TICKER:
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
    case UNDEFINED__:
      break;
    case UNKNOWN__:
      log::warn(R"(Unknown method="{}")"sv, notification.method);
      break;
    case SUBSCRIPTION:
      return json::Parser::dispatch(*this, value, decode_buffer_, trace_info);
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
      auto currencies_update = CurrenciesUpdate{
          .currencies = tmp,
      };
      handler_(currencies_update);
    }
  });
}

bool WebSocket::operator()(Trace<json::Instrument> const &event) {
  if (!master_)
    log::fatal("Unexpected"sv);
  auto &[trace_info, instrument] = event;
  (*connection_).touch(trace_info.source_receive_time);
  log::info<2>("instrument={}"sv, instrument);
  auto &symbol = instrument.instrument_name;
  assert(!std::empty(symbol));
  auto discard = shared_.discard_symbol(symbol);
  // needed by multicast
  auto multiplier = compute_contracts_multiplier(instrument.contract_size);
  auto callback = [&]() -> Instrument {
    if (!discard)
      log::debug(
          R"(CREATE instrument_id={}, instrument_name="{}", contract_size={}, multiplier={})"sv,
          instrument.instrument_id,
          instrument.instrument_name,
          instrument.contract_size,
          multiplier);
    return {
        instrument.instrument_name,
        instrument.contract_size,
        multiplier,
        discard,
    };
  };
  shared_.maybe_create_instrument(instrument.instrument_id, callback);
  if (!shared_.settings.misc.use_fix_reference_data) {
    auto security_type = to_security_type(instrument.kind, instrument.instrument_type, instrument.settlement_period);
    auto min_trade_vol = instrument.min_trade_amount / instrument.contract_size;
    auto trade_vol_step_size = instrument.min_trade_amount / instrument.contract_size;
    auto option_type = to_option_type(instrument.option_type);
    shared_.tick_size_steps.clear();
    for (auto &item : instrument.tick_size_steps) {
      auto tick_size_step = TickSizeStep{
          .min_price = item.above_price,
          .tick_size = item.tick_size,
      };
      shared_.tick_size_steps.emplace_back(std::move(tick_size_step));
    }
    auto reference_data = ReferenceData{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol,
        .description = {},
        .security_type = security_type,
        .cfi_code = {},
        .base_currency = instrument.base_currency,
        .quote_currency = instrument.quote_currency,
        .settlement_currency = instrument.settlement_currency,
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = instrument.tick_size,
        .tick_size_steps = shared_.tick_size_steps,
        .multiplier = instrument.contract_size,
        .min_notional = NaN,
        .min_trade_vol = min_trade_vol,
        .max_trade_vol = NaN,
        .trade_vol_step_size = trade_vol_step_size,
        .option_type = option_type,
        .strike_currency = {},  // XXX FIXME TODO we had this from FIX
        .strike_price = instrument.strike,
        .underlying = instrument.price_index,
        .time_zone = {},
        .issue_date = utils::safe_cast{instrument.creation_timestamp},
        .settlement_date = {},
        .expiry_datetime = {},
        .expiry_datetime_utc = utils::safe_cast{instrument.expiration_timestamp},
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
        .discard = discard,
    };
    create_trace_and_dispatch(handler_, trace_info, reference_data, true);
  }
  if (!discard) {
    // cache multiplier so Quote (amount) can be converted to TopOfBook (lots)
    // note! the multiplier is only cached on startup!
    shared_.multiplier[symbol] = multiplier;
  }
  return discard;
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
