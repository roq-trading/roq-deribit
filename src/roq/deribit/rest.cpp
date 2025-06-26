/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/deribit/rest.hpp"

#include <algorithm>
#include <utility>

#include "roq/logging.hpp"

#include "roq/mask.hpp"

#include "roq/core/json/array_parser.hpp"
#include "roq/core/json/parser.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/web/rest/client.hpp"

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
      decode_buffer_(shared.settings.misc.decode_buffer_size),
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .get_instruments = create_metrics(shared.settings, name_, "get_instruments"sv),
          .get_instruments_ack = create_metrics(shared.settings, name_, "get_instruments_ack"sv),
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
  auto now = event.value.now;
  (*connection_).refresh(now);
  check_download();
}

void Rest::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.get_instruments, metrics::Type::PROFILE)
      .write(profile_.get_instruments_ack, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

void Rest::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::HTTP,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
        .interface = (*connection_).get_interface(),
        .authority = (*connection_).get_current_authority(),
        .path = (*connection_).get_current_path(),
        .proxy = (*connection_).get_proxy(),
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

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
  log::info("Download currencies..."sv);
  auto request = web::rest::Request{
      .method = web::http::Method::GET,
      .path = "/api/v2/public/get_currencies"sv,
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
  (*connection_)("get_currencies"sv, request, callback);
}

void Rest::get_currencies_ack(Trace<web::rest::Response> const &event) {
  auto &[trace_info, response] = event;
  auto handle_success = [&](auto &body) {
    core::json::Parser parser{body};
    auto root = parser.root();
    std::vector<std::string> currencies;
    for (auto [key, value] : std::get<core::json::Object>(root)) {
      if (key == "result"sv) {
        for (auto value_2 : std::get<core::json::Array>(value)) {
          core::json::Buffer buffer{decode_buffer_};
          json::Currency currency{value_2, buffer};
          Trace event_2{trace_info, currency};
          (*this)(event_2);
          // only new
          if (shared_.all_currencies.emplace(currency.currency).second) {
            currencies.emplace_back(currency.currency);
          }
        }
      }
      if (key == "error"sv) {
        json::Error error{value};
        log::error("error={}"sv, error);
      }
    }
    if (!std::empty(currencies)) {
      auto currencies_update = CurrenciesUpdate{
          .currencies = currencies,
      };
      handler_(currencies_update);
    }
    log::info("Currencies download has COMPLETED"sv);
  };
  auto handle_error = [&](auto text) {
    log::error(R"(text="{}")"sv, text);
    log::warn("Currencies download has FAILED"sv);
  };
  process_response(event, handle_success, handle_error);
  request_.respond_currencies = clock::get_system();
  downloading_currencies_ = false;
}

void Rest::operator()(Trace<json::Currency> const &event) {
  auto &[trace_info, currency] = event;
  log::info<2>("currency={}"sv, currency);
}

// instruments

void Rest::get_instruments() {
  log::info("Download instruments..."sv);
  auto request = web::rest::Request{
      .method = web::http::Method::GET,
      .path = "/api/v2/public/get_instruments"sv,
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
  (*connection_)("get_instruments"sv, request, callback);
}

void Rest::get_instruments_ack(Trace<web::rest::Response> const &event) {
  auto &[trace_info, response] = event;
  auto handle_success = [&](auto &body) {
    core::json::Parser parser{body};
    auto root = parser.root();
    std::vector<Symbol> symbols;
    for (auto [key, value] : std::get<core::json::Object>(root)) {
      if (key == "result"sv) {
        for (auto value_2 : std::get<core::json::Array>(value)) {
          core::json::Buffer buffer{decode_buffer_};
          json::Instrument instrument{value_2, buffer};
          Trace event_2{trace_info, instrument};
          // only new
          auto discard = (*this)(event_2);
          if (!discard && shared_.all_symbols.emplace(instrument.instrument_name).second) {
            symbols.emplace_back(instrument.instrument_name);
          }
        }
      }
      if (key == "error"sv) {
        json::Error error{value};
        log::error("error={}"sv, error);
      }
    }
    if (!std::empty(symbols)) {
      auto symbols_update = SymbolsUpdate{
          .symbols = symbols,
      };
      handler_(symbols_update);
    }
    log::info("Instruments download has COMPLETED"sv);
  };
  auto handle_error = [&](auto text) {
    log::error(R"(text="{}")"sv, text);
    log::warn("Instruments download has FAILED"sv);
  };
  process_response(event, handle_success, handle_error);
  request_.respond_instruments = clock::get_system();
  downloading_instruments_ = false;
}

bool Rest::operator()(Trace<json::Instrument> const &event) {
  auto &[trace_info, instrument] = event;
  log::info<2>("instrument={}"sv, instrument);
  auto &symbol = instrument.instrument_name;
  assert(!std::empty(symbol));
  auto discard = shared_.discard_symbol(symbol);
  // needed by multicast
  auto multiplier = compute_contracts_multiplier(instrument.contract_size);
  auto callback = [&]() -> Instrument {
    if (!discard) {
      log::debug(
          R"(CREATE instrument_id={}, instrument_name="{}", contract_size={}, multiplier={})"sv,
          instrument.instrument_id,
          instrument.instrument_name,
          instrument.contract_size,
          multiplier);
    }
    return {
        instrument.instrument_name,
        instrument.contract_size,
        multiplier,
        discard,
    };
  };
  shared_.maybe_create_instrument(instrument.instrument_id, callback);
  if (!shared_.settings.misc.use_fix_reference_data) {
    auto security_type = to_security_type(instrument.kind, instrument.instrument_type, instrument.settlement_period);
    auto min_trade_vol = instrument.min_trade_amount / instrument.contract_size;
    auto trade_vol_step_size = instrument.min_trade_amount / instrument.contract_size;
    auto option_type = to_option_type(instrument.option_type);
    shared_.tick_size_steps.clear();
    for (auto &item : instrument.tick_size_steps) {
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
        .cfi_code = {},
        .base_currency = instrument.base_currency,
        .quote_currency = instrument.quote_currency,
        .settlement_currency = instrument.settlement_currency,
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = instrument.tick_size,
        .tick_size_steps = shared_.tick_size_steps,
        .multiplier = instrument.contract_size,
        .min_notional = NaN,
        .min_trade_vol = min_trade_vol,
        .max_trade_vol = NaN,
        .trade_vol_step_size = trade_vol_step_size,
        .option_type = option_type,
        .strike_currency = {},  // XXX FIXME TODO we had this from FIX
        .strike_price = instrument.strike,
        .underlying = instrument.price_index,
        .time_zone = {},
        .issue_date = utils::safe_cast{instrument.creation_timestamp},
        .settlement_date = {},
        .expiry_datetime = {},
        .expiry_datetime_utc = utils::safe_cast{instrument.expiration_timestamp},
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
        .discard = discard,
    };
    create_trace_and_dispatch(handler_, trace_info, reference_data, true);
  }
  if (!discard) {
    // cache multiplier so Quote (amount) can be converted to TopOfBook (lots)
    // note! the multiplier is only cached on startup!
    shared_.multiplier[symbol] = multiplier;
  }
  return discard;
}

template <typename SuccessHandler, typename ErrorHandler>
void Rest::process_response(web::rest::Response const &response, SuccessHandler success_handler, ErrorHandler error_handler) {
  try {
    auto [status, category, body] = response.result();
    switch (category) {
      using enum web::http::Category;
      case SUCCESS:  // 2xx
        success_handler(body);
        break;
      case CLIENT_ERROR:    // 4xx
      case SERVER_ERROR: {  // 5xx
        auto text = fmt::format("{}"sv, status);
        error_handler(text);
        break;
      }
      default:
        response.expect(web::http::Status::OK);  // throws
    }
  } catch (server::oms::Exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(e.what());
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(e.what());
  }
}

}  // namespace deribit
}  // namespace roq
