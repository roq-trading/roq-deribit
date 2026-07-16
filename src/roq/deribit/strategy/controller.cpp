/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/strategy/controller.hpp"

#include <cassert>

#include "roq/logging.hpp"

#include "roq/clock.hpp"

#include "roq/utils/common.hpp"

#include "roq/io/sys/scheduler.hpp"

#include "roq/deribit/gateway/controller.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace strategy {

// === CONSTANTS ===

namespace {
auto const YIELD_FREQUENCY = 100ms;
size_t const DISPATCH_THIS_MANY_BEFORE_CHECKING_CLOCK = 1000;

auto const WAIT_THIS_LONG_BEFORE_NEXT_STATE_CHANGE = 3s;

// XXX FIXME TODO from flags
auto const ACCOUNT = "A1"sv;
auto const EXCHANGE = "deribit"sv;
auto const SYMBOL = "BTC-PERPETUAL"sv;
auto const QUANTITY = 1.0;
auto const PRICE = 100000.0;
}  // namespace

// === HELPERS ===

namespace {
auto create_dispatcher(auto &handler, auto &settings, auto &config, auto &context) {
  auto helper = [](auto &dispatcher, auto &settings, auto &config, auto &context) {
    return gateway::Controller::create(dispatcher, settings, config, context);
  };
  return std::make_unique<server::Strategy>(handler, settings, config, context, helper);
}
}  // namespace

// === IMPLEMENTATION ===

Controller::Controller(gateway::Settings const &settings, gateway::Config const &config, io::Context &context)
    : settings_{settings}, terminate_{context.create_signal(*this, io::sys::Signal::Type::TERMINATE)},
      interrupt_{context.create_signal(*this, io::sys::Signal::Type::INTERRUPT)}, dispatcher_{create_dispatcher(*this, settings, config, context)} {
}

void Controller::dispatch() {
  (*dispatcher_).start();
  std::chrono::nanoseconds next_yield_ = {};
  auto ok = true;
  while (ok) {
    auto now = clock::get_system();
    refresh(now);
    if (next_yield_ < now && YIELD_FREQUENCY.count() > 0) {
      next_yield_ = now + YIELD_FREQUENCY;
      io::sys::Scheduler::yield();
    }
    for (size_t i = 0; ok && i < DISPATCH_THIS_MANY_BEFORE_CHECKING_CLOCK; ++i) {
      ok = (*dispatcher_).dispatch();
    }
  }
}

void Controller::operator()(State state) {
  assert(state_ != state);
  state_ = state;
  log::info("state={}"sv, state_);
  auto now = clock::get_system();
  next_update_ = now + WAIT_THIS_LONG_BEFORE_NEXT_STATE_CHANGE;
}

void Controller::refresh(std::chrono::nanoseconds now) {
  if (now < next_update_) {
    return;
  }
  switch (state_) {
    using enum State;
    case UNDEFINED:
      // wait for the Ready event
      break;
    case READY:
      (*this)(State::CREATE_ORDER);
      break;
    case CREATE_ORDER:
      (*this)(State::WAITING_CREATE);
      create_order();
      break;
    case WAITING_CREATE:
      log::warn("Request has timed out"sv);
      (*this)(State::DONE);
      break;
    case WORKING:
      (*this)(State::CANCEL_ORDER);
      break;
    case CANCEL_ORDER:
      (*this)(State::WAITING_CANCEL);
      cancel_order();
      break;
    case WAITING_CANCEL:
      log::warn("Request has timed out"sv);
      (*this)(State::DONE);
      break;
    case DONE:
      (*dispatcher_).stop();
      break;
  }
}

void Controller::create_order() {
  assert(order_id_ == 0);
  order_id_ = ++max_order_id_;
  auto create_order = CreateOrder{
      .account = ACCOUNT,  // settings_.account,
      .order_id = order_id_,
      .exchange = EXCHANGE,  // settings_.exchange,
      .symbol = SYMBOL,      // settings_.symbol,
      .side = Side::SELL,
      .position_effect = {},
      .margin_mode = {},
      .quantity_type = {},
      .max_show_quantity = NaN,
      .order_type = OrderType::LIMIT,
      .time_in_force = TimeInForce::GTC,
      .execution_instructions = {},
      .request_template = {},
      .quantity = QUANTITY,  // settings_.quantity,
      .price = PRICE,        // settings_.price,
      .stop_price = NaN,
      .leverage = NaN,
      .routing_id = {},
      .strategy_id = {},
      .release_time_utc = {},
  };
  log::warn("create_order={}"sv, create_order);
  try {
    (*dispatcher_).send(create_order, SOURCE_SELF);
  } catch (NotReady const &e) {
    log::fatal("{}"sv, e);
  }
}

void Controller::cancel_order() {
  assert(order_id_ != 0);
  auto cancel_order = CancelOrder{
      .account = ACCOUNT,  // settings_.account,
      .order_id = order_id_,
      .request_template = {},
      .routing_id = {},
      .version = {},
      .conditional_on_version = {},
      .release_time_utc = {},
  };
  try {
    (*dispatcher_).send(cancel_order, SOURCE_SELF);
  } catch (NotReady const &e) {
    log::fatal("{}"sv, e);
  }
}

// server::Handler2

void Controller::operator()(Trace<HandshakeAck> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<Control> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<DownloadBegin> const &event, uint64_t opaque, bool is_last, uint8_t user_id) {
  auto &[message_info, download_begin] = event;
  log::warn("download_begin={}"sv, download_begin);
}

void Controller::operator()(Trace<DownloadEnd> const &event, uint64_t opaque, bool is_last, uint8_t user_id) {
  auto &[message_info, download_end] = event;
  log::warn("download_end={}"sv, download_end);
  auto max_order_id = download_end.max_order_id;
  if (max_order_id_ < max_order_id) {
    max_order_id_ = max_order_id;
    log::info("max_order_id={}"sv, max_order_id_);
  }
}

void Controller::operator()(Trace<Ready> const &event, uint64_t opaque, bool is_last, uint8_t user_id) {
  auto &[message_info, ready] = event;
  log::warn("ready={}"sv, ready);
  assert(state_ == State::UNDEFINED);
  (*this)(State::READY);
}

void Controller::operator()(Trace<GatewaySettings> const &event, uint64_t opaque, bool is_last, uint8_t user_id) {
  auto &[message_info, gateway_settings] = event;
  log::warn("gateway_settings={}"sv, gateway_settings);
}

void Controller::operator()(Trace<StreamStatus> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<GatewayStatus> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<ExternalLatency> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<RateLimitsUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<RateLimitTrigger> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<ReferenceData> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<MarketStatus> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<TopOfBook> const &event, uint64_t opaque, bool is_last, uint8_t user_id) {
  auto &[message_info, top_of_book] = event;
  // log::warn("top_of_book={}"sv, top_of_book);
}

void Controller::operator()(Trace<MarketByPriceUpdate> const &event, uint64_t opaque, bool is_last, uint8_t user_id) {
  auto &[message_info, market_by_price_update] = event;
  // log::warn("market_by_price_update={}"sv, market_by_price_update);
}

void Controller::operator()(Trace<MarketByOrderUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<TradeSummary> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<StatisticsUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<TimeSeriesUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<CancelAllOrdersAck> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<OrderAck> const &event, uint64_t opaque, bool is_last, uint8_t user_id) {
  auto &[message_info, order_ack] = event;
  log::warn("order_ack={}"sv, order_ack);
  // waiting?
  if (!utils::has_request_maybe_completed(order_ack.request_status)) {
    return;
  }
  // failed?
  if (utils::has_request_failed(order_ack.request_status)) {
    log::warn("Request has failed: status={}"sv, order_ack.request_status);
    (*this)(State::DONE);
    return;
  }
  // success?
  switch (order_ack.request_type) {
    using enum RequestType;
    case UNDEFINED:
      log::fatal("Unexpected"sv);
      break;
    case CREATE_ORDER:
      (*this)(State::WORKING);
      break;
    case MODIFY_ORDER:
      log::fatal("Unexpected"sv);
      break;
    case CANCEL_ORDER:
      (*this)(State::DONE);
      break;
  }
}

void Controller::operator()(Trace<OrderUpdate> const &event, uint64_t opaque, bool is_last, uint8_t user_id) {
  auto &[message_info, order_update] = event;
  log::warn("order_update={}"sv, order_update);
}

void Controller::operator()(Trace<TradeUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<PositionUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

void Controller::operator()(Trace<FundsUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) {
}

// io::sys::Signal::Handler

void Controller::operator()(io::sys::Signal::Event const &event) {
  log::warn("*** SIGNAL: {} ***"sv, event.type);
  (*dispatcher_).stop();
}

}  // namespace strategy
}  // namespace deribit
}  // namespace roq
