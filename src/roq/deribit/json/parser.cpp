/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/json/parser.h"

#include "roq/compat.h"

#include "roq/deribit/json/channel.h"
#include "roq/deribit/json/field.h"
#include "roq/deribit/json/utils.h"

#include "roq/logging.h"

using namespace roq::literals;

namespace roq {
namespace deribit {
namespace json {

namespace {
constexpr std::string_view get_token(const std::string_view &name) {
  auto delim = name.find_first_of('.');
  auto part = name.substr(0, delim);
  if (ROQ_UNLIKELY(part.compare("user"_sv) == 0 && delim != name.npos)) {
    ++delim;
    auto delim_2 = name.find_first_of('.', delim);
    auto length = delim_2 == name.npos ? name.npos : (delim_2 - delim);
    return name.substr(delim, length);
  } else if (ROQ_UNLIKELY(part.compare("instrument"_sv) == 0 && delim != name.npos)) {
    ++delim;
    auto delim_2 = name.find_first_of('.', delim);
    auto length = delim_2 == name.npos ? name.npos : (delim_2 - delim);
    auto name_2 = name.substr(delim, length);
    if (name_2.compare("state"_sv) == 0)
      return "instrument_state"_sv;
  } else {
    return part;
  }
  return ""_sv;
}

static_assert(get_token("ticker"_sv) == "ticker"_sv);
static_assert(get_token("ticker.123"_sv) == "ticker"_sv);
static_assert(get_token("user.changes"_sv) == "changes"_sv);
static_assert(get_token("user.changes.123"_sv) == "changes"_sv);
static_assert(get_token("instrument.state"_sv) == "instrument_state"_sv);
static_assert(get_token("instrument.state.123"_sv) == "instrument_state"_sv);

Channel parse_channel(const std::string_view &name) {
  auto token = get_token(name);
  if (ROQ_UNLIKELY(token.empty()))
    return Channel::UNKNOWN;
  return Channel(token);
}

template <typename T>
void dispatch_platform_state(
    Parser::Handler &handler, T &value, const server::TraceInfo &trace_info) {
  PlatformState platform_state(value);
  server::create_trace_and_dispatch(trace_info, platform_state, handler);
}

template <typename T>
void dispatch_instrument_state(
    Parser::Handler &handler, T &value, const server::TraceInfo &trace_info) {
  InstrumentState instrument_state(value);
  server::create_trace_and_dispatch(trace_info, instrument_state, handler);
}

template <typename T>
void dispatch_quote(Parser::Handler &handler, T &value, const server::TraceInfo &trace_info) {
  Quote quote(value);
  server::create_trace_and_dispatch(trace_info, quote, handler);
}

template <typename T>
void dispatch_ticker(Parser::Handler &handler, T &value, const server::TraceInfo &trace_info) {
  Ticker ticker(value);
  server::create_trace_and_dispatch(trace_info, ticker, handler);
}

template <typename T>
void dispatch_portfolio(Parser::Handler &handler, T &value, const server::TraceInfo &trace_info) {
  Portfolio portfolio(value);
  server::create_trace_and_dispatch(trace_info, portfolio, handler);
}

template <typename T>
void dispatch_changes(
    Parser::Handler &handler,
    T &value,
    core::json::Buffer &buffer,
    const server::TraceInfo &trace_info) {
  Changes changes(value, buffer);
  server::create_trace_and_dispatch(trace_info, changes, handler);
}
}  // namespace

void Parser::dispatch(
    Parser::Handler &handler,
    core::json::value_t &value,
    core::json::Buffer &buffer,
    const server::TraceInfo &trace_info) {
  // note! message is nested / channel name is at level 2
  auto message = core::json::get<std::string_view>(value);
  auto channel = Channel::UNDEFINED;
  bool dispatched = false;
  for (int i = {}; i < 2 && !dispatched; ++i) {
    core::json::Parser parser(message);
    auto root = parser.root();
    for (auto [key, value] : std::get<core::json::object_t>(root)) {
      auto field = Field(key);
      switch (field) {
        case Field::UNDEFINED:
          log::fatal("Unexpected"_sv);
          break;
        case Field::UNKNOWN:
          log::fatal(R"(Unknown key="{}")"_fmt, key);
          break;
        case Field::CHANNEL: {
          auto name = std::get<std::string_view>(value);
          channel = parse_channel(name);
          if (ROQ_UNLIKELY(channel == Channel::UNKNOWN))
            log::warn(R"(Can't parse channel="{}")"_fmt, name);
          break;
        }
        case Field::DATA:
          if (channel != Channel::UNDEFINED) {
            switch (channel) {
              case Channel::UNDEFINED:
                break;  // not ready
              case Channel::UNKNOWN:
                log::fatal("Unknown channel"_sv);
                break;
              // public
              case Channel::PLATFORM_STATE:
                dispatched = true;
                dispatch_platform_state(handler, value, trace_info);
                break;
              case Channel::INSTRUMENT_STATE:
                dispatched = true;
                dispatch_instrument_state(handler, value, trace_info);
                break;
              case Channel::QUOTE:
                dispatched = true;
                dispatch_quote(handler, value, trace_info);
                break;
              case Channel::TICKER:
                dispatched = true;
                dispatch_ticker(handler, value, trace_info);
                break;
              // private
              case Channel::PORTFOLIO:
                dispatched = true;
                dispatch_portfolio(handler, value, trace_info);
                break;
              case Channel::CHANGES:
                dispatched = true;
                dispatch_changes(handler, value, buffer, trace_info);
                break;
            }
          }
          break;
      }
    }
  }
  if (dispatched)
    return;
  log::warn(R"(message="{}")"_fmt, message);
  log::fatal("Unexpected"_sv);
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
