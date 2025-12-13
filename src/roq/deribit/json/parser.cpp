/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/json/parser.hpp"

#include "roq/logging.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/deribit/json/channel.hpp"
#include "roq/deribit/json/field.hpp"
#include "roq/deribit/json/request_type.hpp"
#include "roq/deribit/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace json {

// === CONSTANTS ===

namespace {
constexpr auto const KEY_JSONRPC = "jsonrpc"sv;
constexpr auto const KEY_ID = "id"sv;
constexpr auto const KEY_ERROR = "error"sv;
constexpr auto const KEY_RESULT = "result"sv;
constexpr auto const KEY_METHOD = "method"sv;
constexpr auto const KEY_PARAMS = "params"sv;
//
constexpr auto const KEY_CHANNEL = "channel"sv;

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
bool dispatch_helper(auto &handler, auto &value, auto &buffer_stack, auto &trace_info, Args &&...args) {
  T obj{value, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj, std::forward<Args>(args)...);
  return true;
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
    Parser::Handler &handler,
    std::string_view const &message,
    core::json::BufferStack &buffer_stack,
    TraceInfo const &trace_info,
    bool allow_unknown_event_types) {
  auto result = false;
  auto helper_params = [&](auto &key, auto &value) {
    auto key_2 = utils::hash::FNV::compute(key);
    switch (key_2) {
      case utils::hash::FNV::compute(KEY_CHANNEL): {
        auto name = std::get<std::string_view>(value);
        std::string_view symbol;
        uint32_t interval = {};
        auto channel = parse_channel(name, symbol, interval);
        switch (channel) {
          using enum Channel::type_t;
          case UNDEFINED_INTERNAL:
            break;
          case UNKNOWN_INTERNAL:
            return true;
          // public
          case PLATFORM_STATE:
            result = dispatch_helper<PlatformState>(handler, message, buffer_stack, trace_info);
            return true;
          case INSTRUMENT_STATE:
            result = dispatch_helper<InstrumentState>(handler, message, buffer_stack, trace_info);
            return true;
          case QUOTE:
            result = dispatch_helper<Quote>(handler, message, buffer_stack, trace_info);
            return true;
          case TICKER:
            result = dispatch_helper<Ticker>(handler, message, buffer_stack, trace_info);
            return true;
          case CHART_TRADES:
            result = dispatch_helper<ChartTrades>(handler, message, buffer_stack, trace_info, symbol, interval);
            return true;
          // private
          case PORTFOLIO:
            result = dispatch_helper<UserPortfolio>(handler, message, buffer_stack, trace_info);
            return true;
          case CHANGES:
            result = dispatch_helper<UserChanges>(handler, message, buffer_stack, trace_info);
            return true;
          case ORDERS:
            result = dispatch_helper<UserOrders>(handler, message, buffer_stack, trace_info);
            return true;
          case TRADES:
            result = dispatch_helper<UserTrades>(handler, message, buffer_stack, trace_info);
            return true;
        }
        return true;
      }
    }
    return result;
  };
  auto helper = [&](auto &key, auto &value) {
    auto key_2 = utils::hash::FNV::compute(key);
    switch (key_2) {
      case utils::hash::FNV::compute(KEY_JSONRPC):
        break;
      case utils::hash::FNV::compute(KEY_ID): {
        json::RequestType request_type{std::get<std::string_view>(value)};
        switch (request_type) {
          using enum json::RequestType::type_t;
          case UNDEFINED_INTERNAL:
            break;
          case UNKNOWN_INTERNAL:
            return true;
          case AUTH:
            result = dispatch_helper<Auth>(handler, message, buffer_stack, trace_info);
            return true;
          case SUBSCRIBE_PLATFORM_STATE:
          case SUBSCRIBE_INSTRUMENT_STATE:
          case SUBSCRIBE_QUOTE:
          case SUBSCRIBE_TICKER:
          case SUBSCRIBE_CHART_TRADES:
            result = dispatch_helper<SubscribeAck>(handler, message, buffer_stack, trace_info);
            return true;
          case SUBSCRIBE_USER_PORTFOLIO:
          case SUBSCRIBE_USER_CHANGES:
          case SUBSCRIBE_USER_ORDERS:
          case SUBSCRIBE_USER_TRADES:
            result = dispatch_helper<SubscribeAck>(handler, message, buffer_stack, trace_info);
            return true;
          case GET_ACCOUNT_SUMMARY:
            result = dispatch_helper<GetAccountSummaryAck>(handler, message, buffer_stack, trace_info);
            return true;
          case GET_USER_TRADES_BY_CURRENCY:
            result = dispatch_helper<GetUserTradesByCurrencyAck>(handler, message, buffer_stack, trace_info);
            return true;
        }
        break;
      }
      case utils::hash::FNV::compute(KEY_ERROR):
        break;
      case utils::hash::FNV::compute(KEY_RESULT):
        break;
      case utils::hash::FNV::compute(KEY_METHOD):
        if (std::get<std::string_view>(value) != "subscription"sv) {  // DEBUG
          log::fatal("Unexpected: {}"sv, message);
        }
        break;
      case utils::hash::FNV::compute(KEY_PARAMS):
        std::get<core::json::Object>(value).dispatch(helper_params);
        return true;
    }
    return result;
  };
  core::json::Parser::dispatch<core::json::Object>(helper, message);
  if (result || allow_unknown_event_types) {
    return result;
  }
  log::fatal(R"(Unexpected: message="{}")"sv, message);
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
