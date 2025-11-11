/* Copyright (c) 2017-2025, Hans Erik Thrane */

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

// === CONSTANTS ===

namespace {
constexpr auto const USER = "user"sv;
constexpr auto const INSTRUMENT = "instrument"sv;
constexpr auto const STATE = "state"sv;
constexpr auto const INSTRUMENT_STATE = "instrument_state"sv;
constexpr auto const CHART = "chart"sv;
constexpr auto const TRADES = "trades"sv;
constexpr auto const CHART_TRADES = "chart_trades"sv;
}  // namespace

// === HELPERS ===

namespace {
template <typename T, typename... Args>
void dispatch_helper(auto &handler, auto &value, auto &buffer_stack, auto &trace_info, Args &&...args) {
  T obj{value, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj, std::forward<Args>(args)...);
}

constexpr auto get_token(auto const &name) -> std::string_view {
  auto delim = name.find_first_of('.');
  auto part = name.substr(0, delim);
  if (part == USER && delim != std::string_view::npos) [[unlikely]] {
    ++delim;
    auto delim_2 = name.find_first_of('.', delim);
    auto length = delim_2 == std::string_view::npos ? std::string_view::npos : (delim_2 - delim);
    return name.substr(delim, length);
  } else if (part == INSTRUMENT && delim != std::string_view::npos) [[unlikely]] {
    ++delim;
    auto delim_2 = name.find_first_of('.', delim);
    auto length = delim_2 == std::string_view::npos ? std::string_view::npos : (delim_2 - delim);
    auto name_2 = name.substr(delim, length);
    if (name_2 == STATE) {
      return INSTRUMENT_STATE;
    }
  } else if (part == CHART && delim != std::string_view::npos) [[unlikely]] {
    ++delim;
    auto delim_2 = name.find_first_of('.', delim);
    auto length = delim_2 == std::string_view::npos ? std::string_view::npos : (delim_2 - delim);
    auto name_2 = name.substr(delim, length);
    if (name_2 == TRADES) {
      return CHART_TRADES;
    }
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
static_assert(get_token("chart.trades"sv) == "chart_trades"sv);
static_assert(get_token("chart.trades.123"sv) == "chart_trades"sv);

auto parse_channel(auto const &name, auto &symbol, auto &interval) -> Channel {
  symbol = {};
  interval = {};
  auto token = get_token(name);
  if (std::empty(token)) [[unlikely]] {
    return Channel::UNKNOWN_INTERNAL;
  }
  Channel result{token};
  if (result == Channel::CHART_TRADES) [[unlikely]] {
    auto d1 = name.find_first_of('.');
    if (d1 != std::string_view::npos) {
      auto d2 = name.find_first_of('.', d1 + 1);
      if (d2 != std::string_view::npos) {
        auto d3 = name.find_first_of('.', d2 + 1);
        if (d3 != std::string_view::npos) {
          symbol = name.substr(d2 + 1, d3 - d2 - 1);
          [[maybe_unused]] auto tmp = name.substr(d3 + 1);
        }
      }
    }
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

bool Parser::dispatch(
    Parser::Handler &handler, core::json::Value &value, core::json::BufferStack &buffer_stack, TraceInfo const &trace_info, bool allow_unknown_event_types) {
  // note! message is nested / channel name is at level 2
  auto message = core::json::get<std::string_view>(value);
  auto channel = Channel::UNDEFINED_INTERNAL;
  std::string_view symbol;
  uint32_t interval = {};
  for (int i = 0; i < 2; ++i) {
    core::json::Parser parser{message};
    auto root = parser.root();
    for (auto [key, value_2] : std::get<core::json::Object>(root)) {
      Field field{key};
      switch (field) {
        using enum Field::type_t;
        case UNDEFINED_INTERNAL:
          break;
        case UNKNOWN_INTERNAL:
          if (allow_unknown_event_types) {
            return false;
          }
          break;
        case CHANNEL: {
          auto name = std::get<std::string_view>(value_2);
          channel = parse_channel(name, symbol, interval);
          if (channel == Channel::UNKNOWN_INTERNAL) [[unlikely]] {
            log::warn(R"(Can't parse channel="{}")"sv, name);
          }
          break;
        }
        case DATA:
          switch (channel) {
            using enum Channel::type_t;
            case UNDEFINED_INTERNAL:
              break;
            case UNKNOWN_INTERNAL:
              if (allow_unknown_event_types) {
                return false;
              }
              break;
            // public
            case PLATFORM_STATE:
              dispatch_helper<PlatformState>(handler, value_2, buffer_stack, trace_info);
              return true;
            case INSTRUMENT_STATE:
              dispatch_helper<InstrumentState>(handler, value_2, buffer_stack, trace_info);
              return true;
            case QUOTE:
              dispatch_helper<Quote>(handler, value_2, buffer_stack, trace_info);
              return true;
            case TICKER:
              dispatch_helper<Ticker>(handler, value_2, buffer_stack, trace_info);
              return true;
            case CHART_TRADES:
              dispatch_helper<ChartTrades>(handler, value_2, buffer_stack, trace_info, symbol, interval);
              return true;
            // private
            case PORTFOLIO:
              dispatch_helper<Portfolio>(handler, value_2, buffer_stack, trace_info);
              return true;
            case CHANGES:
              dispatch_helper<Changes>(handler, value_2, buffer_stack, trace_info);
              return true;
            case ORDERS:
              dispatch_helper<Order>(handler, value_2, buffer_stack, trace_info);
              return true;
            case TRADES:
              dispatch_helper<Trades2>(handler, value_2, buffer_stack, trace_info);
              return true;
          }
          break;
      }
    }
  }
  log::fatal(R"(Unexpected: message="{}")"sv, message);
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
