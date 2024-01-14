/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/deribit/drop_copy.hpp"

#include <cppitertools/enumerate.hpp>

#include "roq/mask.hpp"
#include "roq/utils/compare.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/socket/client_factory.hpp"

#include "roq/deribit/json/error.hpp"
#include "roq/deribit/json/method.hpp"
#include "roq/deribit/json/request_type.hpp"
#include "roq/deribit/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const NAME = "ex"sv;

auto const SUPPORTS = Mask{
    SupportType::FUNDS,
};
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id, auto const &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.ws.uri;
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = {},
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.ping_freq,
      // implementation
      .decode_buffer_size = settings.common.decode_buffer_size,
      .encode_buffer_size = settings.common.encode_buffer_size,
  };
  return web::socket::ClientFactory::create(handler, context, config, []() { return std::string(); });
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(auto &settings, auto const &group, auto const &function)
      : core::metrics::Factory(settings.app.name, group, function) {}
};

auto get_download_trades_lookback(auto const &settings, auto download_trades_is_first) {
  if (download_trades_is_first) {
    if (settings.common.download_trades_lookback_on_restart.count())
      return settings.common.download_trades_lookback_on_restart;
  }
  return settings.common.download_trades_lookback;
}
}  // namespace

// === IMPLEMENTATION ===

DropCopy::DropCopy(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account.get_name())},
      connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_(shared.settings.common.decode_buffer_size),
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .auth = create_metrics(shared.settings, name_, "auth"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .heartbeat = create_metrics(shared.settings, name_, "heartbeat"sv),
      },
      account_{account}, shared_{shared},
      download_{shared.settings.ws.request_timeout, [this](auto state) { return download(state); }} {
}

void DropCopy::operator()(Event<Start> const &) {
  (*connection_).start();
}

void DropCopy::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void DropCopy::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
}

void DropCopy::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.auth, metrics::PROFILE)
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void DropCopy::update_subscriptions(std::span<std::string> const &currencies) {
  for (auto &currency : currencies)
    currencies_.emplace_back(currency);
  if (ready_ && can_download_) {
    subscribe_portfolios(currencies);
    get_account_summary(currencies);
    get_trades(currencies);
  }
}

void DropCopy::download() {
  if (can_download_)
    return;
  can_download_ = true;
  if (ready_) {
    subscribe_portfolios(currencies_);
    get_account_summary(currencies_);
    get_trades(currencies_);
  }
}

void DropCopy::operator()(web::socket::Client::Connected const &) {
  // note! wait for upgrade
}

void DropCopy::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void DropCopy::operator()(web::socket::Client::Ready const &) {
  login();
  (*this)(ConnectionStatus::LOGIN_SENT);
}

void DropCopy::operator()(web::socket::Client::Close const &) {
}

void DropCopy::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.get_name(),
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(web::socket::Client::Text const &text) {
  parse(text.payload);
}

void DropCopy::operator()(web::socket::Client::Binary const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = account_.get_name(),
        .supports = SUPPORTS,
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

void DropCopy::login() {
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
      account_.get_access_key(),
      timestamp.count(),
      nonce,
      signature,
      request_type.as_raw_text());
  (*connection_).send_text(message);
}

uint32_t DropCopy::download(DropCopyState state) {
  switch (state) {
    using enum DropCopyState;
    case UNDEFINED:
      break;
    case SUBSCRIBE_PORTFOLIOS:
      if (can_download_)
        subscribe_portfolios(currencies_);
      return {};
    case SUBSCRIBE_CHANGES:
      subscribe_changes();
      return {};
    case SUBSCRIBE_ORDERS:
      subscribe_orders();
      return {};
    case SUBSCRIBE_TRADES:
      subscribe_trades();
      return {};
    case GET_ACCOUNT_SUMMARY:
      if (can_download_)
        get_account_summary(currencies_);
      return {};
    case GET_TRADES:
      if (can_download_)
        get_trades(currencies_);
      return {};
    case DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return {};
  }
  assert(false);
  return {};
}

void DropCopy::subscribe_portfolios(std::span<std::string> const &currencies) {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_PORTFOLIO;
  auto message = fmt::format(
      R"({{)"
      R"("method":"private/subscribe",)"
      R"("params":{{)"
      R"("channels":["user.portfolio.{}"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"sv,
      fmt::join(currencies, R"(","user.portfolio.)"sv),
      request_type.as_raw_text());
  (*connection_).send_text(message);
}

void DropCopy::subscribe_changes() {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_CHANGES;
  auto message = fmt::format(
      R"({{)"
      R"("method":"private/subscribe",)"
      R"("params":{{)"
      R"("channels":["user.changes.any.any.raw"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"sv,
      request_type.as_raw_text());
  (*connection_).send_text(message);
}

void DropCopy::subscribe_orders() {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_ORDERS;
  auto message = fmt::format(
      R"({{)"
      R"("method":"private/subscribe",)"
      R"("params":{{)"
      R"("channels":["user.orders.any.any.raw"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"sv,
      request_type.as_raw_text());
  (*connection_).send_text(message);
}

void DropCopy::subscribe_trades() {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_TRADES;
  auto message = fmt::format(
      R"({{)"
      R"("method":"private/subscribe",)"
      R"("params":{{)"
      R"("channels":["user.trades.any.any.raw"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"sv,
      request_type.as_raw_text());
  (*connection_).send_text(message);
}

void DropCopy::get_account_summary(std::span<std::string> const &currencies) {
  constexpr json::RequestType request_type = json::RequestType::GET_ACCOUNT_SUMMARY;
  for (auto currency : currencies) {
    auto message = fmt::format(
        R"({{)"
        R"("method":"private/get_account_summary",)"
        R"("params":{{)"
        R"("currency":"{}",)"
        R"("extended":true)"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"sv,
        currency,
        request_type.as_raw_text());
    (*connection_).send_text(message);
  }
}

// XXX TODO specify subaccount_id
void DropCopy::get_trades(std::span<std::string> const &currencies) {
  constexpr json::RequestType request_type = json::RequestType::GET_TRADES;
  auto now = clock::get_realtime<std::chrono::milliseconds>();
  auto lookback = get_download_trades_lookback(shared_.settings, download_trades_is_first_);
  log::info<1>("Download trades: lookback={}"sv, lookback);
  auto start_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - lookback);
  for (auto currency : currencies) {
    auto message = fmt::format(
        R"({{)"
        R"("method":"private/get_user_trades_by_currency",)"
        R"("params":{{)"
        R"("currency":"{}",)"
        R"("start_timestamp":{})"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"sv,
        currency,
        start_timestamp.count(),
        request_type.as_raw_text());
    (*connection_).send_text(message);
  }
}

void DropCopy::parse(std::string_view const &message) {
  profile_.parse([&]() {
    try {
      TraceInfo trace_info;
      core::jsonrpc::Parser::dispatch(*this, message, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void DropCopy::operator()(Trace<core::jsonrpc::Error> const &event, core::json::Value &value) {
  auto &[trace_info, error] = event;
  json::Error error_2{value};
  if (shared_.settings.ws.allow_errors)
    log::warn(R"(error={}, id="{}")"sv, error_2, error.id);
  else
    log::fatal(R"(error={}, id="{}")"sv, error_2, error.id);
}

void DropCopy::operator()(Trace<core::jsonrpc::Result> const &event, core::json::Value &value) {
  auto &[trace_info, result] = event;
  json::RequestType request_type{result.id};
  switch (request_type) {
    using enum json::RequestType::type_t;
    case UNDEFINED__:
      break;
    case UNKNOWN__:
      log::fatal(R"(Unknown request_type="{}")"sv, result.id);
      return;
    case AUTH: {
      json::Auth const auth{value};
      Trace event{trace_info, auth};
      (*this)(event);
      return;
    }
    case GET_CURRENCIES:
    case GET_INSTRUMENTS:
    case SUBSCRIBE_PLATFORM_STATE:
    case SUBSCRIBE_INSTRUMENT_STATE:
    case SUBSCRIBE_QUOTE:
    case SUBSCRIBE_TICKER:
      break;  // unexpected
    case SUBSCRIBE_PORTFOLIO:
    case SUBSCRIBE_CHANGES:
    case SUBSCRIBE_ORDERS:
    case SUBSCRIBE_TRADES:
      // note! no need to parse
      return;
    case GET_ACCOUNT_SUMMARY: {
      auto portfolio = json::Portfolio{value};
      create_trace_and_dispatch(*this, trace_info, portfolio);
      return;
    }
    case GET_TRADES: {
      core::json::Buffer buffer{decode_buffer_};
      auto trades = json::Trades{value, buffer};
      create_trace_and_dispatch(*this, trace_info, trades);
      return;
    }
    case GET_POSITIONS:
      break;  // unexpected
  }
  log::fatal("Unexpected: request_type={}"sv, request_type);
}

void DropCopy::operator()(Trace<core::jsonrpc::Notification> const &event, core::json::Value &value) {
  auto &[trace_info, notification] = event;
  json::Method method{notification.method};
  switch (method) {
    using enum json::Method::type_t;
    case UNDEFINED__:
      break;
    case UNKNOWN__:
      log::fatal(R"(Unknown method="{}")"sv, notification.method);
      break;
    case SUBSCRIPTION:
      json::Parser::dispatch(*this, value, decode_buffer_, trace_info);
      break;
  }
}

void DropCopy::operator()(Trace<json::Auth> const &event) {
  profile_.auth([&]() {
    auto &[trace_info, auth] = event;
    log::info<2>("auth={}"sv, auth);
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  });
}

void DropCopy::operator()(Trace<json::PlatformState> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::InstrumentState> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Quote> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Ticker> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Portfolio> const &event) {
  log::info<2>("portfolio={}"sv, event.value);
  auto &[trace_info, portfolio] = event;
  auto margin_mode = portfolio.cross_collateral_enabled ? MarginMode::CROSS : MarginMode::ISOLATED;
  auto funds_update = FundsUpdate{
      .stream_id = stream_id_,
      .account = account_.get_name(),
      .currency = portfolio.currency,
      .margin_mode = margin_mode,
      .balance = portfolio.balance,
      .hold = NaN,
      .external_account = {},
      .update_type = UpdateType::INCREMENTAL,
      .exchange_time_utc = portfolio.creation_timestamp,
      .sending_time_utc = {},
  };
  create_trace_and_dispatch(handler_, event.trace_info, funds_update, true);
}

// note! includes trades
void DropCopy::operator()(Trace<json::Changes> const &event) {
  auto &[trace_info, changes] = event;
  auto &trades = changes.trades;
  for (auto &&[i, trade] : iter::enumerate(trades)) {
    auto is_last = i == (std::size(trades) - 1);
    create_trace_and_dispatch(*this, event.trace_info, std::as_const(trade), false, is_last);
  }
}

void DropCopy::operator()(Trace<json::Trades> const &event) {
  auto &[trace_info, trades] = event;
  auto &trades_2 = trades.trades;
  for (auto &&[i, trade] : iter::enumerate(trades_2)) {
    auto is_last = i == (std::size(trades_2) - 1);
    create_trace_and_dispatch(*this, event.trace_info, std::as_const(trade), true, is_last);
  }
  download_trades_is_first_ = false;
}

void DropCopy::operator()(Trace<json::Order> const &event) {
  auto &[trace_info, order] = event;
  log::info<1>("order={}"sv, order);
  // do nothing?
}

// note! managed by changes
void DropCopy::operator()(Trace<json::Trades2> const &event) {
  auto &[trace_info, trades2] = event;
  log::info<1>("trades={}"sv, trades2);
  // do nothing?
}

// XXX maybe drop this an aggregate by order?
// note! trade.label might be our ClOrdID
void DropCopy::operator()(Trace<json::Trade> const &event, bool is_download, bool is_last) {
  auto &trace_info = event.trace_info;
  auto &trade = event.value;
  log::info<1>("trade={}"sv, trade);
  auto side = json::map(trade.direction);
  auto iter = shared_.multiplier.find(trade.instrument_name);
  if (iter == std::end(shared_.multiplier))
    log::warn(R"(*** NO MULTIPLIER FOR SYMBOL="{}" ***)"sv, trade.instrument_name);
  auto multiplier = iter == std::end(shared_.multiplier) ? 1.0 : (*iter).second;  // XXX not good
  auto quantity = trade.amount * multiplier;
  auto liquidity = json::map(trade.liquidity);
  auto fill = Fill{
      .exchange_time_utc = trade.timestamp,
      .external_trade_id = {},
      .quantity = quantity,
      .price = trade.price,
      .liquidity = liquidity,
  };
  // note! this is consistent with FIX (there is also a trade_id field, but it's not consistent)
  fmt::format_to(std::back_inserter(fill.external_trade_id), "{}#{}"sv, trade.instrument_name, trade.trade_seq);
  auto update_type = is_download ? UpdateType::SNAPSHOT : UpdateType::INCREMENTAL;
  auto trade_update = TradeUpdate{
      .stream_id = stream_id_,
      .account = account_.get_name(),
      .order_id = {},
      .exchange = shared_.settings.exchange,
      .symbol = trade.instrument_name,
      .side = side,
      .position_effect = {},
      .margin_mode = {},
      .create_time_utc = trade.timestamp,
      .update_time_utc = trade.timestamp,
      .external_account = {},
      .external_order_id = trade.order_id,
      .client_order_id = {},
      .fills = {&fill, 1},
      .routing_id = {},
      .update_type = update_type,
      .sending_time_utc = {},
      .user = {},
      .strategy_id = {},
  };
  create_trace_and_dispatch(handler_, trace_info, trade_update, is_last, SOURCE_NONE, trade.label);
}

}  // namespace deribit
}  // namespace roq
