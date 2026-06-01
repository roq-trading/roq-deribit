/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/strategy/controller.hpp"

#include <fmt/format.h>

#include <magic_enum/magic_enum_format.hpp>

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
auto const WAIT_THIS_LONG_BEFORE_NEXT_STATE_CHANGE = 10s;
}  // namespace

// === IMPLEMENTATION ===

Controller::Controller(gateway::Settings const &settings, gateway::Config const &config, io::Context &context)
    : server::Strategy{
          *this,
          settings,
          config,
          context,
          [](auto &dispatcher, auto &settings, auto &config, auto &context) { return gateway::Controller::create(dispatcher, settings, config, context); }},
      settings_{settings}, terminate_{context.create_signal(*this, io::sys::Signal::Type::TERMINATE)},
      interrupt_{context.create_signal(*this, io::sys::Signal::Type::INTERRUPT)} {
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
      stop();
      break;
  }
}

void Controller::create_order() {
  /*
  assert(order_id_ == 0);
  order_id_ = ++max_order_id_;
  auto create_order = CreateOrder{
      .account = settings_.account,
      .order_id = order_id_,
      .exchange = settings_.exchange,
      .symbol = settings_.symbol,
      .side = Side::BUY,
      .position_effect = {},
      .margin_mode = {},
      .quantity_type = {},
      .max_show_quantity = NaN,
      .order_type = OrderType::LIMIT,
      .time_in_force = TimeInForce::GTC,
      .execution_instructions = {},
      .request_template = {},
      .quantity = settings_.quantity,
      .price = settings_.price,
      .stop_price = NaN,
      .leverage = NaN,
      .routing_id = {},
      .strategy_id = {},
      .release_time_utc = {},
  };
  try {
    (*dispatcher_).send(create_order, 0);
  } catch (NotConnected const &e) {
    log::fatal("{}"sv, e);
  } catch (NotReady const &e) {
    log::fatal("{}"sv, e);
  }
  */
}

void Controller::cancel_order() {
  /*
  assert(order_id_ != 0);
  auto cancel_order = CancelOrder{
      .account = settings_.account,
      .order_id = order_id_,
      .request_template = {},
      .routing_id = {},
      .version = {},
      .conditional_on_version = {},
      .release_time_utc = {},
  };
  try {
    (*dispatcher_).send(cancel_order, 0);
  } catch (NotConnected const &e) {
    log::fatal("{}"sv, e);
  } catch (NotReady const &e) {
    log::fatal("{}"sv, e);
  }
  */
}

// server::Strategy::Handler

void Controller::operator()(Event<DownloadBegin> const &event) {
  auto &[message_info, download_begin] = event;
  log::warn("download_begin={}"sv, download_begin);
}

void Controller::operator()(Event<DownloadEnd> const &event) {
  auto &[message_info, download_end] = event;
  log::warn("download_end={}"sv, download_end);
  auto max_order_id = download_end.max_order_id;
  if (max_order_id_ < max_order_id) {
    max_order_id_ = max_order_id;
    log::info("max_order_id={}"sv, max_order_id_);
  }
}

void Controller::operator()(Event<Ready> const &event) {
  auto &[message_info, ready] = event;
  log::warn("ready={}"sv, ready);
  // assert(state_ == State::CONNECTING);
  (*this)(State::READY);
}

void Controller::operator()(Event<GatewaySettings> const &event) {
  auto &[message_info, gateway_settings] = event;
  log::warn("gateway_settings={}"sv, gateway_settings);
}

void Controller::operator()(Event<StreamStatus> const &) {
}

void Controller::operator()(Event<GatewayStatus> const &) {
}

void Controller::operator()(Event<ReferenceData> const &) {
}

void Controller::operator()(Event<MarketStatus> const &) {
}

void Controller::operator()(Event<TopOfBook> const &event) {
  auto &[message_info, top_of_book] = event;
  // log::warn("top_of_book={}"sv, top_of_book);
}

void Controller::operator()(Event<MarketByPriceUpdate> const &event) {
  auto &[message_info, market_by_price_update] = event;
  // log::warn("market_by_price_update={}"sv, market_by_price_update);
}

void Controller::operator()(Event<OrderAck> const &event) {
  log::debug("event={}"sv, event);
  auto &order_ack = event.value;
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

void Controller::operator()(Event<OrderUpdate> const &event) {
  log::debug("event={}"sv, event);
}

// io::sys::Signal::Handler

void Controller::operator()(io::sys::Signal::Event const &event) {
  log::warn("*** SIGNAL: {} ***"sv, event.type);
  stop();
}

}  // namespace strategy
}  // namespace deribit
}  // namespace roq
