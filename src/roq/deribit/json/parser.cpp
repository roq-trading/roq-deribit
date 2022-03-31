/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/json/parser.hpp"

#include "roq/compat.hpp"

#include "roq/deribit/json/channel.hpp"
#include "roq/deribit/json/field.hpp"
#include "roq/deribit/json/utils.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace json {

namespace {
constexpr std::string_view get_token(const std::string_view &name) {
  auto delim = name.find_first_of('.');
  auto part = name.substr(0, delim);
  if (part.compare("user"sv) == 0 && delim != name.npos) [[unlikely]] {
    ++delim;
    auto delim_2 = name.find_first_of('.', delim);
    auto length = delim_2 == name.npos ? name.npos : (delim_2 - delim);
    return name.substr(delim, length);
  } else if (part.compare("instrument"sv) == 0 && delim != name.npos) [[unlikely]] {
    ++delim;
    auto delim_2 = name.find_first_of('.', delim);
    auto length = delim_2 == name.npos ? name.npos : (delim_2 - delim);
    auto name_2 = name.substr(delim, length);
    if (name_2.compare("state"sv) == 0)
      return "instrument_state"sv;
  } else {
    return part;
  }
  return ""sv;
}

static_assert(get_token("ticker"sv) == "ticker"sv);
static_assert(get_token("ticker.123"sv) == "ticker"sv);
static_assert(get_token("user.changes"sv) == "changes"sv);
static_assert(get_token("user.changes.123"sv) == "changes"sv);
static_assert(get_token("instrument.state"sv) == "instrument_state"sv);
static_assert(get_token("instrument.state.123"sv) == "instrument_state"sv);

Channel parse_channel(const std::string_view &name) {
  auto token = get_token(name);
  if (std::empty(token)) [[unlikely]]
    return Channel::UNKNOWN;
  return Channel(token);
}

template <typename T>
void dispatch_platform_state(Parser::Handler &handler, T &value, const TraceInfo &trace_info) {
  PlatformState platform_state(value);
  create_trace_and_dispatch(handler, trace_info, platform_state);
}

template <typename T>
void dispatch_instrument_state(Parser::Handler &handler, T &value, const TraceInfo &trace_info) {
  InstrumentState instrument_state(value);
  create_trace_and_dispatch(handler, trace_info, instrument_state);
}

template <typename T>
void dispatch_quote(Parser::Handler &handler, T &value, const TraceInfo &trace_info) {
  Quote quote(value);
  create_trace_and_dispatch(handler, trace_info, quote);
}

template <typename T>
void dispatch_ticker(Parser::Handler &handler, T &value, const TraceInfo &trace_info) {
  Ticker ticker(value);
  create_trace_and_dispatch(handler, trace_info, ticker);
}

template <typename T>
void dispatch_portfolio(Parser::Handler &handler, T &value, const TraceInfo &trace_info) {
  Portfolio portfolio(value);
  create_trace_and_dispatch(handler, trace_info, portfolio);
}

template <typename T>
void dispatch_changes(
    Parser::Handler &handler, T &value, core::json::Buffer &buffer, const TraceInfo &trace_info) {
  Changes changes(value, buffer);
  create_trace_and_dispatch(handler, trace_info, changes);
}

template <typename T>
void dispatch_orders(
    Parser::Handler &handler, T &value, core::json::Buffer &, const TraceInfo &trace_info) {
  Order order(value);
  create_trace_and_dispatch(handler, trace_info, order);
}

template <typename T>
void dispatch_trades(
    Parser::Handler &handler, T &value, core::json::Buffer &buffer, const TraceInfo &trace_info) {
  Trades2 trades(value, buffer);
  create_trace_and_dispatch(handler, trace_info, trades);
}
}  // namespace

void Parser::dispatch(
    Parser::Handler &handler,
    core::json::value_t &value,
    core::json::Buffer &buffer,
    const TraceInfo &trace_info) {
  // note! message is nested / channel name is at level 2
  auto message = core::json::get<std::string_view>(value);
  auto channel = Channel::UNDEFINED;
  bool dispatched = false;
  for (int i = 0; i < 2 && !dispatched; ++i) {
    core::json::Parser parser(message);
    auto root = parser.root();
    for (auto [key, value_] : std::get<core::json::object_t>(root)) {
      auto field = Field(key);
      switch (field) {
        case Field::UNDEFINED:
          log::fatal("Unexpected"sv);
          break;
        case Field::UNKNOWN:
          log::fatal(R"(Unknown key="{}")"sv, key);
          break;
        case Field::CHANNEL: {
          auto name = std::get<std::string_view>(value_);
          channel = parse_channel(name);
          if (channel == Channel::UNKNOWN) [[unlikely]]
            log::warn(R"(Can't parse channel="{}")"sv, name);
          break;
        }
        case Field::DATA:
          if (channel != Channel::UNDEFINED) {
            switch (channel) {
              case Channel::UNDEFINED:
                break;  // not ready
              case Channel::UNKNOWN:
                log::fatal("Unknown channel"sv);
                break;
              // public
              case Channel::PLATFORM_STATE:
                dispatched = true;
                dispatch_platform_state(handler, value_, trace_info);
                break;
              case Channel::INSTRUMENT_STATE:
                dispatched = true;
                dispatch_instrument_state(handler, value_, trace_info);
                break;
              case Channel::QUOTE:
                dispatched = true;
                dispatch_quote(handler, value_, trace_info);
                break;
              case Channel::TICKER:
                dispatched = true;
                dispatch_ticker(handler, value_, trace_info);
                break;
              // private
              case Channel::PORTFOLIO:
                dispatched = true;
                dispatch_portfolio(handler, value_, trace_info);
                break;
              case Channel::CHANGES:
                dispatched = true;
                dispatch_changes(handler, value_, buffer, trace_info);
                break;
              case Channel::ORDERS:
                dispatched = true;
                dispatch_orders(handler, value_, buffer, trace_info);
                break;
              case Channel::TRADES:
                dispatched = true;
                dispatch_trades(handler, value_, buffer, trace_info);
                break;
            }
          }
          break;
      }
    }
  }
  if (dispatched)
    return;
  log::warn(R"(message="{}")"sv, message);
  log::fatal("Unexpected"sv);
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
