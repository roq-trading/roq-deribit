/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/drop_copy.h"

#include "roq/utils/compare.h"
#include "roq/utils/mask.h"
#include "roq/utils/safe_cast.h"
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
static const auto NAME = "ex"sv;
static const auto SUPPORTS = utils::Mask{
    SupportType::FUNDS,
    SupportType::POSITION,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

DropCopy::DropCopy(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared)
    : handler_(handler), stream_id_(stream_id),
      name_(fmt::format("{}:{}:{}"sv, stream_id_, NAME, security.get_account())),
      connection_(
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
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
      },
      security_(security), shared_(shared),
      download_(Flags::ws_request_timeout(), [this](auto state) { return download(state); }) {
}

void DropCopy::operator()(const Event<Start> &) {
  connection_.start();
}

void DropCopy::operator()(const Event<Stop> &) {
  connection_.stop();
}

void DropCopy::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

uint16_t DropCopy::operator()(
    const Event<CreateOrder> &, [[maybe_unused]] const std::string_view &request_id) {
  throw oms::NotSupportedException();
  return stream_id_;
}

uint16_t DropCopy::operator()(
    const Event<ModifyOrder> &,
    const oms::Order &,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  throw oms::NotSupportedException();
  return stream_id_;
}

uint16_t DropCopy::operator()(
    const Event<CancelOrder> &,
    const oms::Order &,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  throw oms::NotSupportedException();
  return stream_id_;
}

uint16_t DropCopy::operator()(const Event<CancelAllOrders> &) {
  throw oms::NotSupportedException();
  return stream_id_;
}

void DropCopy::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.auth, metrics::PROFILE)
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void DropCopy::update_subscriptions(const roq::span<std::string> &currencies) {
  for (auto &currency : currencies)
    currencies_.emplace_back(currency);
  if (ready_) {
    subscribe_portfolios(currencies);
    get_account_summary(currencies);
    get_trades(currencies);
  }
}

void DropCopy::operator()(const core::web::ClientSocket::Connected &) {
  // note! wait for upgrade
}

void DropCopy::operator()(const core::web::ClientSocket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void DropCopy::operator()(const core::web::ClientSocket::Ready &) {
  login();
  (*this)(ConnectionStatus::LOGIN_SENT);
}

void DropCopy::operator()(const core::web::ClientSocket::Close &) {
}

void DropCopy::operator()(const core::web::ClientSocket::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(const core::web::ClientSocket::Text &text) {
  parse(text.payload);
}

void DropCopy::operator()(const core::web::ClientSocket::Binary &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void DropCopy::login() {
  constexpr json::RequestType request_type = json::RequestType::AUTH;
  std::chrono::milliseconds now = utils::safe_cast(core::get_realtime_clock());
  auto nonce = security_.create_nonce();
  auto [signature, timestamp] = security_.create_signature(now, nonce);
  log::info(
      "DEBUG: HASHER stream_id={}, real={}, used={}, diff={}"sv,
      stream_id_,
      now,
      timestamp,
      (timestamp - now));
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
      security_.get_access_key(),
      timestamp.count(),
      nonce,
      signature,
      request_type.as_raw_text());
  connection_.send_text(message);
}

uint32_t DropCopy::download(DropCopyState state) {
  switch (state) {
    case DropCopyState::UNDEFINED:
      break;
    case DropCopyState::SUBSCRIBE_PORTFOLIOS:
      subscribe_portfolios(currencies_);
      return {};
    case DropCopyState::SUBSCRIBE_CHANGES:
      subscribe_changes();
      return {};
    case DropCopyState::SUBSCRIBE_ORDERS:
      subscribe_orders();
      return {};
    case DropCopyState::SUBSCRIBE_TRADES:
      subscribe_trades();
      return {};
    case DropCopyState::GET_ACCOUNT_SUMMARY:
      get_account_summary(currencies_);
      return {};
    case DropCopyState::GET_TRADES:
      get_trades(currencies_);
      return {};
    case DropCopyState::DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return {};
  }
  assert(false);
  return {};
}

void DropCopy::subscribe_portfolios(const roq::span<std::string> &currencies) {
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
  connection_.send_text(message);
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
  connection_.send_text(message);
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
  connection_.send_text(message);
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
  connection_.send_text(message);
}

void DropCopy::get_account_summary(const roq::span<std::string> &currencies) {
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
    connection_.send_text(message);
  }
}

void DropCopy::get_trades(const roq::span<std::string> &currencies) {
  constexpr json::RequestType request_type = json::RequestType::GET_TRADES;
  for (auto currency : currencies) {
    auto message = fmt::format(
        R"({{)"
        R"("method":"private/get_user_trades_by_currency",)"
        R"("params":{{)"
        R"("currency":"{}",)"
        R"("count":{})"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"sv,
        currency,
        Flags::ws_max_trades(),
        request_type.as_raw_text());
    connection_.send_text(message);
  }
}

void DropCopy::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      auto trace_info = server::create_trace_info();
      core::jsonrpc::Parser::dispatch(*this, message, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void DropCopy::operator()(
    const server::Trace<core::jsonrpc::Error> &event, core::json::value_t &value) {
  auto &[trace_info, error] = event;
  json::Error error_2(value);
  log::fatal(R"(error={}, id="{}")"sv, error_2, error.id);
}

void DropCopy::operator()(
    const server::Trace<core::jsonrpc::Result> &event, core::json::value_t &value) {
  auto &[trace_info, result] = event;
  json::RequestType request_type(result.id);
  switch (request_type) {
    case json::RequestType::UNDEFINED:
      break;
    case json::RequestType::UNKNOWN:
      log::fatal(R"(Unknown request_type="{}")"sv, result.id);
      break;
    case json::RequestType::AUTH: {
      json::Auth auth(value);
      server::Trace event(trace_info, auth);
      (*this)(event);
      break;
    }
    case json::RequestType::SUBSCRIBE_PORTFOLIO:
    case json::RequestType::SUBSCRIBE_CHANGES:
    case json::RequestType::SUBSCRIBE_ORDERS:
    case json::RequestType::SUBSCRIBE_TRADES:
      break;
    case json::RequestType::GET_ACCOUNT_SUMMARY: {
      json::Portfolio portfolio(value);
      server::create_trace_and_dispatch(*this, trace_info, portfolio);
      break;
    }
    case json::RequestType::GET_TRADES: {
      core::json::Buffer buffer(decode_buffer_);
      json::Trades trades(value, buffer);
      server::create_trace_and_dispatch(*this, trace_info, trades);
      break;
    }
    default:
      log::fatal("Unexpected: request_type={}"sv, request_type);
  }
}

void DropCopy::operator()(
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

void DropCopy::operator()(const server::Trace<json::Auth> &event) {
  profile_.auth([&]() {
    auto &[trace_info, auth] = event;
    log::info<2>("auth={}"sv, auth);
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  });
}

void DropCopy::operator()(const server::Trace<json::PlatformState> &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(const server::Trace<json::InstrumentState> &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(const server::Trace<json::Quote> &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(const server::Trace<json::Ticker> &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(const server::Trace<json::Portfolio> &event) {
  log::info<2>("portfolio={}"sv, event.value);
  auto &portfolio = event.value;
  FundsUpdate funds_update{
      .stream_id = stream_id_,
      .account = security_.get_account(),
      .currency = portfolio.currency,
      .balance = portfolio.balance,
      .hold = NaN,
      .external_account = {},
  };
  server::create_trace_and_dispatch(handler_, event.trace_info, funds_update, true);
}

void DropCopy::operator()(const server::Trace<json::Changes> &event) {
  auto &trades = event.value.trades;
  for (size_t i = {}; i < trades.size(); ++i) {
    auto &trade = trades[i];
    auto is_last = i == (trades.size() - 1);
    server::create_trace_and_dispatch(*this, event.trace_info, trade, is_last);
  }
}

void DropCopy::operator()(const server::Trace<json::Trades> &event) {
  auto &trades = event.value.trades;
  for (size_t i = {}; i < trades.size(); ++i) {
    auto &trade = trades[i];
    auto is_last = i == (trades.size() - 1);
    server::create_trace_and_dispatch(*this, event.trace_info, trade, is_last);
  }
}

void DropCopy::operator()(const server::Trace<json::Order> &event) {
  log::info<1>("order={}"sv, event.value);
  // do nothing?
}

void DropCopy::operator()(const server::Trace<json::Trades2> &event) {
  log::info<1>("trades={}"sv, event.value);
  // do nothing?
}

void DropCopy::operator()(const server::Trace<json::Trade> &event, [[maybe_unused]] bool is_last) {
  log::info<1>("trade={}"sv, event.value);
  // do nothing?
}

}  // namespace deribit
}  // namespace roq
