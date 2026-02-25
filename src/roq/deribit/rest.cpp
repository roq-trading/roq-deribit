/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/rest.hpp"

#include <algorithm>
#include <utility>

#include "roq/logging.hpp"

#include "roq/mask.hpp"

#include "roq/core/json/array_parser.hpp"
#include "roq/core/json/parser.hpp"

#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/deribit/utils.hpp"

#include "roq/deribit/json/error.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const NAME = "rest"sv;

auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 2;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.rest.uri;
  auto config = web::rest::Client::Config{
      // connection
      .interface = {},
      .proxy = settings.rest.proxy,
      .uris = {&uri, 1},
      .host = settings.rest.host,
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = {},
      .disconnect_on_idle_timeout = {},
      .connection = web::http::Connection::KEEP_ALIVE,
      // request
      .allow_pipelining = true,
      .request_timeout = settings.rest.request_timeout,
      // response
      .suspend_on_retry_after = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .ping_frequency = settings.rest.ping_freq,
      .ping_path = settings.rest.ping_path,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::rest::Client::create(handler, context, config);
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};

auto to_security_type(auto kind, [[maybe_unused]] auto instrument_type, auto settlement_period) -> SecurityType {
  switch (kind) {
    using enum json::Kind::type_t;
    case UNDEFINED_INTERNAL:
    case UNKNOWN_INTERNAL:
      return {};
    case FUTURE:
      /*
      switch (instrument_type) {
        using enum json::InstrumentType::type_t;
        case UNDEFINED_INTERNAL:
        case UNKNOWN_INTERNAL:
          return SecurityType::FUTURES;
        case LINEAR:
        case REVERSED:
          return SecurityType::SWAP;
      }
      */
      switch (settlement_period) {
        using enum json::SettlementPeriod::type_t;
        case UNDEFINED_INTERNAL:
        case UNKNOWN_INTERNAL:
          return SecurityType::FUTURES;
        case PERPETUAL:
          return SecurityType::SWAP;
        case DAY:
        case WEEK:
        case MONTH:
        case HOUR:
          return SecurityType::FUTURES;
      }
      break;
    case OPTION:
      return SecurityType::OPTION;
    case FUTURE_COMBO:
      return SecurityType::FUTURES;  // ???
    case OPTION_COMBO:
      return SecurityType::OPTION;  // ???
    case SPOT:
      return SecurityType::SPOT;
  }
  log::fatal("Unexpected"sv);
}

auto to_option_type(auto option_type) -> OptionType {
  switch (option_type) {
    using enum json::OptionType::type_t;
    case UNDEFINED_INTERNAL:
    case UNKNOWN_INTERNAL:
      return {};
    case CALL:
      return OptionType::CALL;
    case PUT:
      return OptionType::PUT;
  }
  log::fatal("Unexpected"sv);
}
}  // namespace

// === IMPLEMENTATION ===

Rest::Rest(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, Request &request)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .currencies = create_metrics(shared.settings, name_, "currencies"sv),
          .currencies_ack = create_metrics(shared.settings, name_, "currencies_ack"sv),
          .instruments = create_metrics(shared.settings, name_, "instruments"sv),
          .instruments_ack = create_metrics(shared.settings, name_, "instruments_ack"sv),
          .chart_data = create_metrics(shared.settings, name_, "chart_data"sv),
          .chart_data_ack = create_metrics(shared.settings, name_, "chart_data_ack"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      shared_{shared}, request_{request} {
}

void Rest::operator()(Event<Start> const &) {
  (*connection_).start();
}

void Rest::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void Rest::operator()(Event<Timer> const &event) {
  auto &[message_info, timer] = event;
  (*connection_).refresh(timer.now);
  check_download();
  if (ready()) {
    check_request_queue(timer.now);
  }
}

void Rest::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.currencies, metrics::Type::PROFILE)
      .write(profile_.currencies_ack, metrics::Type::PROFILE)
      .write(profile_.instruments, metrics::Type::PROFILE)
      .write(profile_.instruments_ack, metrics::Type::PROFILE)
      .write(profile_.chart_data, metrics::Type::PROFILE)
      .write(profile_.chart_data_ack, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

void Rest::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = {},
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::HTTP,
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

// web::rest::Client::Handler

void Rest::operator()(Trace<web::rest::Client::Connected> const &) {
  assert(!downloading());
  (*this)(ConnectionStatus::READY);
  check_download();
}

void Rest::operator()(Trace<web::rest::Client::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  downloading_currencies_ = false;
  downloading_instruments_ = false;
}

void Rest::operator()(Trace<web::rest::Client::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void Rest::check_download() {
  if (!ready()) {
    return;
  }
  if (!downloading() && request_.respond_currencies < request_.request_currencies) {
    get_currencies();
    downloading_currencies_ = true;
  }
  if (!downloading() && request_.respond_instruments < request_.request_instruments) {
    get_instruments();
    downloading_instruments_ = true;
  }
}

// currencies

void Rest::get_currencies() {
  profile_.currencies([&]() {
    log::info("Download currencies..."sv);
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = shared_.api.market_data.currencies,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_currencies_ack(event);
    };
    (*connection_)("currencies"sv, request, callback);
  });
}

void Rest::get_currencies_ack(Trace<web::rest::Response> const &event) {
  profile_.currencies_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      log::warn("Currencies download has FAILED"sv);
    };
    auto handle_success = [&](auto &body) {
      json::GetCurrenciesAck currencies_ack{body, decode_buffer_};
      if (currencies_ack.error.code == 0) {
        Trace event_2{event, currencies_ack};
        (*this)(event_2);
      } else {
        handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, currencies_ack.error.message);
      }
      log::info("Currencies download has COMPLETED"sv);
    };
    process_response(event, handle_error, handle_success);
    request_.respond_currencies = clock::get_system();
    downloading_currencies_ = false;
  });
}

void Rest::operator()(Trace<json::GetCurrenciesAck> const &event) {
  auto &[trace_info, currencies_ack] = event;
  log::info<2>("currencies_ack={}"sv, currencies_ack);
  std::vector<std::string> currencies;
  for (auto &item : currencies_ack.result) {
    // only new
    if (shared_.all_currencies.emplace(item.currency).second) {
      currencies.emplace_back(item.currency);
    }
  }
  if (!std::empty(currencies)) {
    auto currencies_update = CurrenciesUpdate{
        .currencies = currencies,
    };
    handler_(currencies_update);
  }
}

// instruments

void Rest::get_instruments() {
  profile_.instruments([&]() {
    log::info("Download instruments..."sv);
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = shared_.api.market_data.instruments,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_instruments_ack(event);
    };
    (*connection_)("instruments"sv, request, callback);
  });
}

void Rest::get_instruments_ack(Trace<web::rest::Response> const &event) {
  profile_.instruments_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      log::warn("Instruments download has FAILED"sv);
    };
    auto handle_success = [&](auto &body) {
      json::GetInstrumentsAck instruments_ack{body, decode_buffer_};
      if (instruments_ack.error.code == 0) {
        Trace event_2{event, instruments_ack};
        (*this)(event_2);
      } else {
        handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, instruments_ack.error.message);
      }
      log::info("Instruments download has COMPLETED"sv);
    };
    process_response(event, handle_error, handle_success);
    request_.respond_instruments = clock::get_system();
    downloading_instruments_ = false;
  });
}

void Rest::operator()(Trace<json::GetInstrumentsAck> const &event) {
  auto &[trace_info, instruments_ack] = event;
  log::info<2>("instruments_ack={}"sv, instruments_ack);
  std::vector<Symbol> symbols;
  for (auto &item : instruments_ack.result) {
    auto &symbol = item.instrument_name;
    assert(!std::empty(symbol));
    auto discard = shared_.discard_symbol(symbol);
    // needed by multicast
    auto multiplier = compute_contracts_multiplier(item.contract_size);
    auto callback = [&]() -> Instrument {
      if (!discard) {
        log::debug(
            R"(CREATE instrument_id={}, instrument_name="{}", contract_size={}, multiplier={})"sv,
            item.instrument_id,
            item.instrument_name,
            item.contract_size,
            multiplier);
      }
      return {
          item.instrument_name,
          item.contract_size,
          multiplier,
          discard,
      };
    };
    shared_.maybe_create_instrument(item.instrument_id, callback);
    if (!shared_.settings.misc.use_fix_reference_data) {
      auto security_type = to_security_type(item.kind, item.instrument_type, item.settlement_period);
      auto min_trade_vol = item.min_trade_amount / item.contract_size;
      auto trade_vol_step_size = item.min_trade_amount / item.contract_size;
      auto option_type = to_option_type(item.option_type);
      shared_.tick_size_steps.clear();
      for (auto &item : item.tick_size_steps) {
        auto tick_size_step = TickSizeStep{
            .min_price = item.above_price,
            .tick_size = item.tick_size,
        };
        shared_.tick_size_steps.emplace_back(tick_size_step);  // XXX FIXME std::move
      }
      auto reference_data = ReferenceData{
          .stream_id = stream_id_,
          .exchange = shared_.settings.exchange,
          .symbol = symbol,
          .description = {},
          .security_type = security_type,
          .external_security_id = {},
          .cfi_code = {},
          .base_currency = item.base_currency,
          .quote_currency = item.quote_currency,
          .settlement_currency = item.settlement_currency,
          .margin_currency = {},
          .commission_currency = {},
          .tick_size = item.tick_size,
          .tick_size_steps = shared_.tick_size_steps,
          .multiplier = item.contract_size,
          .min_notional = NaN,
          .min_trade_vol = min_trade_vol,
          .max_trade_vol = NaN,
          .trade_vol_step_size = trade_vol_step_size,
          .option_type = option_type,
          .strike_currency = {},  // XXX FIXME TODO we had this from FIX
          .strike_price = item.strike,
          .underlying = item.price_index,
          .time_zone = {},
          .issue_date = utils::safe_cast{item.creation_timestamp},
          .settlement_date = {},
          .expiry_datetime = {},
          .expiry_datetime_utc = utils::safe_cast{item.expiration_timestamp},
          .exchange_time_utc = {},
          .exchange_sequence = {},
          .sending_time_utc = {},
          .discard = discard,
      };
      create_trace_and_dispatch(handler_, trace_info, reference_data, true);
    }
    // cache multiplier so Quote (amount) can be converted to TopOfBook (lots)
    // note! the multiplier is only cached on startup!
    shared_.multiplier[symbol] = multiplier;
    // only new
    if (!discard && shared_.all_symbols.emplace(item.instrument_name).second) {
      symbols.emplace_back(item.instrument_name);
    }
  }
  if (!std::empty(symbols)) {
    auto symbols_update = SymbolsUpdate{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
}

// chart-data

void Rest::get_chart_data(std::string_view const &symbol) {
  profile_.chart_data([&]() {
    auto end_time = clock::get_realtime<std::chrono::milliseconds>();
    auto start_time = end_time - shared_.settings.time_series.lookback;
    auto query = fmt::format(
        "?instrument_name={}"
        "&start_timestamp={}"
        "&end_timestamp={}"
        "&resolution=1"sv,
        symbol,
        start_time.count(),
        end_time.count());
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = shared_.api.market_data.chart_data,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, symbol = std::string{symbol}]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_chart_data_ack(event, symbol);
    };
    (*connection_)("chart-data"sv, request, callback);
  });
}

void Rest::get_chart_data_ack(Trace<web::rest::Response> const &event, std::string_view const &symbol) {
  profile_.chart_data_ack([&]() {
    auto &[trace_info, response] = event;
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      log::warn("Chart-data download has FAILED"sv);
    };
    auto handle_success = [&](auto &body) {
      json::GetChartDataAck chart_data_ack{body, decode_buffer_};
      if (chart_data_ack.error.code == 0) {
        Trace event_2{trace_info, chart_data_ack};
        (*this)(event_2, symbol);
      } else {
        handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, chart_data_ack.error.message);
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void Rest::operator()(Trace<json::GetChartDataAck> const &event, std::string_view const &symbol) {
  auto &[trace_info, chart_data_ack] = event;
  log::info<2>(R"(chart_data_ack={}, symbol="{}")"sv, chart_data_ack, symbol);
  auto &result = chart_data_ack.result;
  auto &bars = shared_.bars;
  bars.clear();
  auto length = std::size(result.ticks);
  // TODO check length of arrays
  for (size_t i = 0; i < length; ++i) {
    auto begin_time_utc = std::chrono::milliseconds{result.ticks[i]};
    auto bar = Bar{
        .begin_time_utc = utils::safe_cast(begin_time_utc),
        .confirmed = true,
        .open_price = result.open[i],
        .high_price = result.high[i],
        .low_price = result.low[i],
        .close_price = result.close[i],
        .quantity = result.volume[i],
        .base_amount = NaN,
        .quote_amount = NaN,
        .number_of_trades = {},
        .vwap = NaN,
    };
    bars.emplace_back(std::move(bar));
  }
  auto time_series_update = TimeSeriesUpdate{
      .stream_id = stream_id_,
      .exchange = shared_.settings.exchange,
      .symbol = symbol,
      .data_source = DataSource::TRADE_SUMMARY,
      .interval = shared_.settings.time_series.interval,
      .origin = Origin::EXCHANGE,
      .bars = bars,
      .update_type = UpdateType::SNAPSHOT,
      .exchange_time_utc = {},  // XXX FIXME
  };
  create_trace_and_dispatch(handler_, trace_info, time_series_update, true);
}

// helpers

void Rest::check_request_queue(std::chrono::nanoseconds now) {
  auto can_request = [&](auto now) { return shared_.rate_limiter.can_request(now); };
  auto request = [&](auto &symbol) { get_chart_data(symbol); };
  shared_.time_series_request_queue.dispatch(can_request, request, now);
}

void Rest::process_response(web::rest::Response const &response, auto error_handler, auto success_handler) {
  try {
    auto [status, category, body] = response.result();
    switch (category) {
      using enum web::http::Category;
      case UNKNOWN:
      case INFORMATIONAL_RESPONSE:
        response.expect(web::http::Status::OK);  // throws
        break;
      case SUCCESS:
        success_handler(body);
        break;
      case REDIRECTION:
        log::fatal("Unexpected: URL is being redirected"sv);
      case CLIENT_ERROR:
      case SERVER_ERROR: {
        auto message = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, message);
        break;
      }
    }
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::GATEWAY, e.request_status(), e.error(), e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, e.what());
  }
}

}  // namespace deribit
}  // namespace roq
