/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/drop_copy.hpp"

#include "roq/mask.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/compare.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/web/socket/client.hpp"

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
auto const NAME = "ex"sv;

auto const SUPPORTS = Mask{
    SupportType::FUNDS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 1;
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
      .interface = settings.misc.test_local_interface,
      .uris = {&uri, 1},
      .host = settings.ws.host,
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = {},
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
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

auto get_download_trades_lookback(auto const &settings, auto download_trades_is_first) {
  if (download_trades_is_first) {
    if (settings.download.trades_lookback_on_restart.count()) {
      return settings.download.trades_lookback_on_restart;
    }
  }
  return settings.download.trades_lookback;
}
}  // namespace

// === IMPLEMENTATION ===

DropCopy::DropCopy(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account.name)}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
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
      account_{account}, shared_{shared}, download_{shared.settings.ws.request_timeout, [this](auto state) { return download(state); }} {
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

void DropCopy::operator()(metrics::Writer &writer) const {
  writer  //
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.auth, metrics::Type::PROFILE)
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

void DropCopy::update_subscriptions(std::span<std::string> const &currencies) {
  for (auto &currency : currencies) {
    currencies_.emplace_back(currency);
  }
  if (ready_ && can_download_) {
    subscribe_user_portfolio(currencies);
    get_account_summary(currencies);
    get_user_trades_by_currency(currencies);
  }
}

void DropCopy::download() {
  if (can_download_) {
    return;
  }
  can_download_ = true;
  if (ready_) {
    subscribe_user_portfolio(currencies_);
    get_account_summary(currencies_);
    get_user_trades_by_currency(currencies_);
  }
}

// web::socket::Client::Parser::Handler

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
      .account = account_.name,
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

void DropCopy::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = account_.name,
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::WS,
      .encoding = {Encoding::JSON},
      .priority = Priority::PRIMARY,
      .connection_status = connection_status_,
      .reason = reason,
      .interface = (*connection_).get_interface(),
      .authority = (*connection_).get_current_authority(),
      .path = (*connection_).get_current_path(),
      .proxy = (*connection_).get_proxy(),
  };
  log::info("stream_status={}"sv, stream_status);
  create_trace_and_dispatch(handler_, trace_info, stream_status);
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
      account_.key,
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
    case SUBSCRIBE_USER_PORTFOLIO:
      if (can_download_) {
        (*this)(ConnectionStatus::DOWNLOADING, "subscribe-user-portfolio"sv);
        subscribe_user_portfolio(currencies_);
      }
      return 0;
    case SUBSCRIBE_USER_CHANGES:
      (*this)(ConnectionStatus::DOWNLOADING, "subscribe-user-changes"sv);
      subscribe_user_changes();
      return 0;
    case SUBSCRIBE_USER_ORDERS:
      (*this)(ConnectionStatus::DOWNLOADING, "subscribe-user-orders"sv);
      subscribe_user_orders();
      return 0;
    case SUBSCRIBE_USER_TRADES:
      (*this)(ConnectionStatus::DOWNLOADING, "subscribe-user-trades"sv);
      subscribe_user_trades();
      return 0;
    case GET_ACCOUNT_SUMMARY:
      if (can_download_) {
        (*this)(ConnectionStatus::DOWNLOADING, "account-summary"sv);
        get_account_summary(currencies_);
      }
      return 0;
    case GET_USER_TRADES_BY_CURRENCY:
      if (can_download_) {
        (*this)(ConnectionStatus::DOWNLOADING, "user-trades-by-currency"sv);
        get_user_trades_by_currency(currencies_);
      }
      return 0;
    case DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return 0;
  }
  assert(false);
  return 0;
}

void DropCopy::subscribe_user_portfolio(std::span<std::string> const &currencies) {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_USER_PORTFOLIO;
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

void DropCopy::subscribe_user_changes() {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_USER_CHANGES;
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

void DropCopy::subscribe_user_orders() {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_USER_ORDERS;
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

void DropCopy::subscribe_user_trades() {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_USER_TRADES;
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
void DropCopy::get_user_trades_by_currency(std::span<std::string> const &currencies) {
  constexpr json::RequestType request_type = json::RequestType::GET_USER_TRADES_BY_CURRENCY;
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
    // log::debug("{}"sv, message);
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    try {
      TraceInfo trace_info;
      if (!json::Parser::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

// json::Parser::Handler

void DropCopy::operator()(Trace<json::Auth> const &event) {
  profile_.auth([&]() {
    auto &[trace_info, auth] = event;
    log::info<2>("auth={}"sv, auth);
    download_.begin();
  });
}

void DropCopy::operator()(Trace<json::SubscribeAck> const &event) {
  auto &[trace_info, subscribe_ack] = event;
  if (subscribe_ack.error.code != 0) {
    log::error("subscribe_ack={}"sv, subscribe_ack);
  }
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

void DropCopy::operator()(Trace<json::ChartTrades> const &, [[maybe_unused]] std::string_view const &symbol, [[maybe_unused]] uint32_t interval) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::UserPortfolio> const &event) {
  auto &[trace_info, user_portfolio] = event;
  log::info<2>("user_portfolio={}"sv, user_portfolio);
  auto &data = user_portfolio.params.data;
  auto margin_mode = data.cross_collateral_enabled ? MarginMode::CROSS : MarginMode::ISOLATED;
  auto funds_update = FundsUpdate{
      .stream_id = stream_id_,
      .account = account_.name,
      .currency = data.currency,
      .margin_mode = margin_mode,
      .balance = data.balance,
      .hold = NaN,
      .borrowed = NaN,
      .unrealized_pnl = NaN,
      .external_account = {},
      .update_type = UpdateType::INCREMENTAL,
      .exchange_time_utc = data.creation_timestamp,
      .sending_time_utc = {},
  };
  create_trace_and_dispatch(handler_, event.trace_info, funds_update, true);
}

// note! includes trades
void DropCopy::operator()(Trace<json::UserChanges> const &event) {
  auto &[trace_info, user_changes] = event;
  auto &trades = user_changes.params.data.trades;
  for (size_t i = 0; i < std::size(trades); ++i) {
    auto &trade = trades[i];
    auto is_last = i == (std::size(trades) - 1);
    create_trace_and_dispatch(*this, event.trace_info, std::as_const(trade), false, is_last);
  }
}

void DropCopy::operator()(Trace<json::UserTrades> const &event) {
  auto &[trace_info, user_trades] = event;
  log::info<1>("user_trades={}"sv, user_trades);
  auto &trades = user_trades.params.data;
  for (size_t i = 0; i < std::size(trades); ++i) {
    auto &trade = trades[i];
    auto is_last = i == (std::size(trades) - 1);
    create_trace_and_dispatch(*this, event.trace_info, std::as_const(trade), true, is_last);
  }
  download_trades_is_first_ = false;
}

void DropCopy::operator()(Trace<json::UserOrders> const &event) {
  auto &[trace_info, user_orders] = event;
  log::info<1>("user_orders={}"sv, user_orders);
  // do nothing?
}

// note! not using -- already managed by user-changes
void DropCopy::operator()(Trace<json::GetAccountSummaryAck> const &event) {
  auto &[trace_info, get_account_summary_ack] = event;
  log::info<1>("get_account_summary_ack={}"sv, get_account_summary_ack);
  // do nothing?
}

// note! not using -- already managed by user-changes
void DropCopy::operator()(Trace<json::GetUserTradesByCurrencyAck> const &event) {
  auto &[trace_info, get_user_trades_by_currency_ack] = event;
  log::info<1>("get_user_trades_by_currency_ack={}"sv, get_user_trades_by_currency_ack);
  // do nothing?
}

// XXX maybe drop this an aggregate by order?
// note! trade.label might be our ClOrdID
void DropCopy::operator()(Trace<json::Trade> const &event, bool is_download, bool is_last) {
  auto &[trace_info, trade] = event;
  log::info<1>("trade={}"sv, trade);
  auto iter = shared_.multiplier.find(trade.instrument_name);
  if (iter == std::end(shared_.multiplier)) {
    log::warn(R"(*** NO MULTIPLIER FOR SYMBOL="{}" ***)"sv, trade.instrument_name);
  }
  // XXX warning ... cached multiplier is 1/x of ref_data
  auto multiplier = iter == std::end(shared_.multiplier) ? 1.0 : (*iter).second;  // XXX not good
  auto quantity = trade.amount * multiplier;
  auto side = map(trade.direction).template get<Side>();
  auto ref_data = shared_.get_ref_data(shared_.settings.exchange, trade.instrument_name);
  auto profit_loss_amount = utils::compute_profit_loss_amount(side, quantity, trade.price, ref_data.multiplier);
  log::debug("multiplier: cached={}, ref_data={}"sv, multiplier, ref_data.multiplier);
  auto fill = Fill{
      .exchange_time_utc = trade.timestamp,
      .external_trade_id = {},
      .quantity = quantity,
      .price = trade.price,
      .liquidity = map(trade.liquidity),
      .commission_amount = trade.fee,
      .commission_currency = trade.fee_currency,
      .base_amount = NaN,   // XXX TODO spot?
      .quote_amount = NaN,  // XXX TODO spot?
      .profit_loss_amount = profit_loss_amount,
  };
  // note! this is consistent with FIX (there is also a trade_id field, but it's not consistent)
  fmt::format_to(std::back_inserter(fill.external_trade_id), "{}#{}"sv, trade.instrument_name, trade.trade_seq);
  log::debug("fill={}"sv, fill);
  auto update_type = is_download ? UpdateType::SNAPSHOT : UpdateType::INCREMENTAL;
  auto trade_update = TradeUpdate{
      .stream_id = stream_id_,
      .account = account_.name,
      .order_id = {},
      .exchange = shared_.settings.exchange,
      .symbol = trade.instrument_name,
      .side = side,
      .position_effect = {},
      .margin_mode = {},  // XXX FIXME TODO
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
