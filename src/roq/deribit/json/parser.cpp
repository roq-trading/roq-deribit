/* Copyright (c) 2017-2024, Hans Erik Thrane */

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

// === HELPERS ===

namespace {
constexpr auto get_token(auto const &name) -> std::string_view {
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
  return {};
}

static_assert(get_token("ticker"sv) == "ticker"sv);
static_assert(get_token("ticker.123"sv) == "ticker"sv);
static_assert(get_token("user.changes"sv) == "changes"sv);
static_assert(get_token("user.changes.123"sv) == "changes"sv);
static_assert(get_token("instrument.state"sv) == "instrument_state"sv);
static_assert(get_token("instrument.state.123"sv) == "instrument_state"sv);

auto parse_channel(auto const &name) -> Channel {
  auto token = get_token(name);
  if (std::empty(token)) [[unlikely]]
    return Channel::UNKNOWN__;
  return Channel{token};
}
}  // namespace

// === IMPLEMENTATION ===

bool Parser::dispatch(
    Parser::Handler &handler,
    core::json::Value &value,
    std::span<std::byte> const &buffer,
    TraceInfo const &trace_info) {
  // note! message is nested / channel name is at level 2
  auto message = core::json::get<std::string_view>(value);
  auto channel = Channel::UNDEFINED__;
  bool dispatched = false;
  for (int i = 0; i < 2 && !dispatched; ++i) {
    core::json::Parser parser{message};
    auto root = parser.root();
    for (auto [key, value_] : std::get<core::json::Object>(root)) {
      Field field{key};
      switch (field) {
        using enum Field::type_t;
        case UNDEFINED__:
          log::fatal("Unexpected"sv);
          break;
        case UNKNOWN__:
          log::warn(R"(Unknown key="{}")"sv, key);
          return false;
        case CHANNEL: {
          auto name = std::get<std::string_view>(value_);
          channel = parse_channel(name);
          if (channel == Channel::UNKNOWN__) [[unlikely]]
            log::warn(R"(Can't parse channel="{}")"sv, name);
          break;
        }
        case DATA:
          if (channel != Channel::UNDEFINED__) {
            core::json::Buffer buffer_2{buffer};
            switch (channel) {
              using enum Channel::type_t;
              case UNDEFINED__:
                break;  // not ready
              case UNKNOWN__:
                log::warn("Unknown channel"sv);
                return false;
              // public
              case PLATFORM_STATE: {
                dispatched = true;
                PlatformState platform_state{value_, buffer_2};
                create_trace_and_dispatch(handler, trace_info, platform_state);
                break;
              }
              case INSTRUMENT_STATE: {
                dispatched = true;
                InstrumentState instrument_state{value_, buffer_2};
                create_trace_and_dispatch(handler, trace_info, instrument_state);
                break;
              }
              case QUOTE: {
                dispatched = true;
                Quote quote{value_, buffer_2};
                create_trace_and_dispatch(handler, trace_info, quote);
                break;
              }
              case TICKER: {
                dispatched = true;
                Ticker ticker{value_, buffer_2};
                create_trace_and_dispatch(handler, trace_info, ticker);
                break;
              }
              // private
              case PORTFOLIO: {
                dispatched = true;
                Portfolio portfolio{value_, buffer_2};
                create_trace_and_dispatch(handler, trace_info, portfolio);
                break;
              }
              case CHANGES: {
                dispatched = true;
                Changes changes{value_, buffer_2};
                create_trace_and_dispatch(handler, trace_info, changes);
                break;
              }
              case ORDERS: {
                dispatched = true;
                Order order{value_, buffer_2};
                create_trace_and_dispatch(handler, trace_info, order);
                break;
              }
              case TRADES: {
                dispatched = true;
                Trades2 trades{value_, buffer_2};
                create_trace_and_dispatch(handler, trace_info, trades);
                break;
              }
            }
          }
          break;
      }
    }
  }
  return dispatched;
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
