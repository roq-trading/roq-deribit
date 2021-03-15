/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/drop_copy.h"

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
static const auto CONNECTION = "ex"_sv;

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
      name_(roq::format("{}:{}:{}"_fmt, stream_id_, CONNECTION, security.get_account())),
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
          .positions = create_metrics(name_, "positions"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
          .heartbeat = create_metrics(name_, "heartbeat"_sv),
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

void DropCopy::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.auth, metrics::PROFILE)
      .write(profile_.positions, metrics::PROFILE)
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
    get_positions(currencies);
  }
}

void DropCopy::operator()(const core::web::Socket::Connected &) {
  // note! wait for upgrade
}

void DropCopy::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(GatewayStatus::DISCONNECTED);
  download_.reset();
}

void DropCopy::operator()(const core::web::Socket::Ready &) {
  login();
  (*this)(GatewayStatus::LOGIN_SENT);
}

void DropCopy::operator()(const core::web::Socket::Close &) {
}

void DropCopy::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .name = name_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(trace_info, external_latency, handler_);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void DropCopy::operator()(GatewayStatus status) {
  if (core::update(status_, status)) {
    server::TraceInfo trace_info;
    OrderManagerStatus order_manager_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .status = status_,
    };
    LOG(INFO)("order_manager_status={}"_fmt, order_manager_status);
    server::create_trace_and_dispatch(trace_info, order_manager_status, handler_);
  }
}

void DropCopy::login() {
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
    case DropCopyState::GET_ACCOUNT_SUMMARY:
      get_account_summary(currencies_);
      return {};
    case DropCopyState::GET_TRADES:
      get_trades(currencies_);
      return {};
    case DropCopyState::GET_POSITIONS:
      get_positions(currencies_);
      return {};
    case DropCopyState::DONE:
      (*this)(GatewayStatus::READY);
      assert(!ready_);
      ready_ = true;
      return {};
  }
  assert(false);
  return {};
}

void DropCopy::subscribe_portfolios(const roq::span<std::string> &currencies) {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_PORTFOLIO;
  auto message = roq::format(
      R"({{)"
      R"("method":"private/subscribe",)"
      R"("params":{{)"
      R"("channels":["user.portfolio.{}"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_fmt,
      roq::join(currencies, R"(,user.portfolio.)"_sv),
      request_type.as_raw_text());
  connection_.send_text(message);
}

void DropCopy::subscribe_changes() {
  constexpr json::RequestType request_type = json::RequestType::SUBSCRIBE_CHANGES;
  auto message = roq::format(
      R"({{)"
      R"("method":"private/subscribe",)"
      R"("params":{{)"
      R"("channels":["user.changes.any.any.raw"])"
      R"(}},)"
      R"("id":"{}")"
      R"(}})"_fmt,
      request_type.as_raw_text());
  connection_.send_text(message);
}

void DropCopy::get_account_summary(const roq::span<std::string> &currencies) {
  constexpr json::RequestType request_type = json::RequestType::GET_ACCOUNT_SUMMARY;
  for (auto currency : currencies) {
    auto message = roq::format(
        R"({{)"
        R"("method":"private/get_account_summary",)"
        R"("params":{{)"
        R"("currency":"{}",)"
        R"("extended":true)"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"_fmt,
        currency,
        request_type.as_raw_text());
    connection_.send_text(message);
  }
}

void DropCopy::get_trades(const roq::span<std::string> &currencies) {
  constexpr json::RequestType request_type = json::RequestType::GET_TRADES;
  for (auto currency : currencies) {
    auto message = roq::format(
        R"({{)"
        R"("method":"private/get_user_trades_by_currency",)"
        R"("params":{{)"
        R"("currency":"{}",)"
        R"("count":{})"
        R"(}},)"
        R"("id":"{}")"
        R"(}})"_fmt,
        currency,
        Flags::ws_max_trades(),
        request_type.as_raw_text());
    connection_.send_text(message);
  }
}

void DropCopy::get_positions(const roq::span<std::string> &currencies) {
  constexpr json::RequestType request_type = json::RequestType::GET_POSITIONS;
  for (auto currency : currencies) {
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
}

void DropCopy::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      core::jsonrpc::Parser::dispatch(*this, message);
    } catch (std::exception &e) {
      LOG(WARNING)(R"(message="{}")"_fmt, message);
      LOG(FATAL)(R"("ERROR what="{}")"_fmt, e.what());
    }
  });
}

void DropCopy::operator()(const core::jsonrpc::Error &error, core::json::value_t &value) {
  json::Error error_2(value);
  LOG(FATAL)(R"(error={}, id="{}")"_fmt, error_2, error.id);
}

void DropCopy::operator()(const core::jsonrpc::Result &result, core::json::value_t &value) {
  server::TraceInfo trace_info;  // XXX not correct (*parsing* has already started)
  json::RequestType request_type(result.id);
  switch (request_type) {
    case json::RequestType::UNDEFINED:
      break;
    case json::RequestType::UNKNOWN:
      DLOG(FATAL)(R"(DEBUG: Unknown request_type="{}")"_fmt, result.id);
      break;
    case json::RequestType::AUTH: {
      json::Auth auth(value);
      (*this)(auth, trace_info);
      break;
    }
    case json::RequestType::SUBSCRIBE_PORTFOLIO: {
      break;
    }
    case json::RequestType::SUBSCRIBE_CHANGES: {
      break;
    }
    case json::RequestType::GET_ACCOUNT_SUMMARY: {
      json::Portfolio portfolio(value);
      server::create_trace_and_dispatch(trace_info, portfolio, *this);
      break;
    }
    case json::RequestType::GET_TRADES: {
      core::json::Buffer buffer(decode_buffer_);
      json::Trades trades(value, buffer);
      server::create_trace_and_dispatch(trace_info, trades, *this);
      break;
    }
    case json::RequestType::GET_POSITIONS: {
      core::json::Buffer buffer(decode_buffer_);
      json::Positions positions(value, buffer);
      server::create_trace_and_dispatch(trace_info, positions, *this);
      break;
    }
    default:
      LOG(FATAL)("Unexpected: request_type={}"_fmt, request_type);
  }
}

void DropCopy::operator()(
    const core::jsonrpc::Notification &notification, core::json::value_t &value) {
  server::TraceInfo trace_info;  // XXX not correct (*parsing* has already started)
  json::Method method(notification.method);
  switch (method) {
    case json::Method::UNDEFINED:
      break;
    case json::Method::UNKNOWN:
      DLOG(FATAL)(R"(DEBUG: Unknown method="{}")"_fmt, notification.method);
      break;
    case json::Method::SUBSCRIPTION: {
      core::json::Buffer buffer(decode_buffer_);
      json::Parser::dispatch(*this, value, buffer, trace_info);
      break;
    }
  }
}

void DropCopy::operator()(const json::Auth &auth, const server::TraceInfo &) {
  profile_.auth([&]() {
    VLOG(1)(R"(auth={})"_fmt, auth);
    (*this)(GatewayStatus::DOWNLOADING);
    download_.begin();
  });
}

void DropCopy::operator()(const server::Trace<json::PlatformState> &) {
  LOG(FATAL)("Unexpected"_sv);
}

void DropCopy::operator()(const server::Trace<json::InstrumentState> &) {
  LOG(FATAL)("Unexpected"_sv);
}

void DropCopy::operator()(const server::Trace<json::Quote> &) {
  LOG(FATAL)("Unexpected"_sv);
}

void DropCopy::operator()(const server::Trace<json::Ticker> &) {
  LOG(FATAL)("Unexpected"_sv);
}

void DropCopy::operator()(const server::Trace<json::Portfolio> &event) {
  auto &portfolio = event.value;
  FundsUpdate funds_update{
      .stream_id = stream_id_,
      .account = security_.get_account(),
      .currency = portfolio.currency,
      .balance = portfolio.balance,
      .hold = NaN,
      .external_account = {},
  };
  server::create_trace_and_dispatch(event.trace_info, funds_update, handler_, true);
}

void DropCopy::operator()(const server::Trace<json::Changes> &event) {
  auto &trades = event.value.trades;
  for (size_t i = {}; i < trades.size(); ++i) {
    auto &trade = trades[i];
    auto is_last = i == (trades.size() - 1);
    server::create_trace_and_dispatch(event.trace_info, trade, *this, is_last);
  }
}

void DropCopy::operator()(const server::Trace<json::Trades> &event) {
  auto &trades = event.value.trades;
  for (size_t i = {}; i < trades.size(); ++i) {
    auto &trade = trades[i];
    auto is_last = i == (trades.size() - 1);
    server::create_trace_and_dispatch(event.trace_info, trade, *this, is_last);
  }
}

void DropCopy::operator()(const server::Trace<json::Positions> &event) {
  auto &positions = event.value;
  for (size_t i = {}; i < positions.data.size(); ++i) {
    auto &position = positions.data[i];
    auto is_last = i == (positions.data.size() - 1);
    server::create_trace_and_dispatch(event.trace_info, position, *this, is_last);
  }
}

void DropCopy::operator()(const server::Trace<json::Trade> &event, [[maybe_unused]] bool is_last) {
  DLOG(INFO)("DEBUG: trade={}"_fmt, event.value);
  // do nothing?
}

void DropCopy::operator()(const server::Trace<json::Position> &event, bool is_last) {
  auto &position = event.value;
  PositionUpdate position_update{
      .stream_id = stream_id_,
      .account = security_.get_account(),
      .exchange = Flags::exchange(),
      .symbol = position.instrument_name,
      .side = json::map(position.direction),
      .position = position.size,
      .last_trade_id = {},
      .position_cost = 0.0,
      .position_yesterday = 0.0,
      .position_cost_yesterday = 0.0,
      .external_account = {},
  };
  server::create_trace_and_dispatch(event.trace_info, position_update, handler_, is_last);
}

}  // namespace deribit
}  // namespace roq
