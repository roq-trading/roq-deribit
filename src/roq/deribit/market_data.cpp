/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/market_data.h"

#include <algorithm>

#include "roq/utils/compare.h"
#include "roq/utils/mask.h"
#include "roq/utils/safe_cast.h"
#include "roq/utils/update.h"

#include "roq/core/back_emplacer.h"
#include "roq/core/debug.h"

#include "roq/core/charconv/datetime.h"

#include "roq/core/metrics/factory.h"

#include "roq/core/fix/utils.h"

#include "roq/deribit/common.h"
#include "roq/deribit/flags.h"

#include "roq/deribit/fix/utils.h"

using namespace roq::literals;

namespace roq {
namespace deribit {

namespace {
static const auto LOGOUT_RESPONSE = "LOGOUT"_sv;  // XXX

static const auto NAME = "md"_sv;
static const auto SUPPORTS = utils::Mask{
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
    SupportType::STATISTICS,
};
static const auto SUPPORTS_MASTER = utils::Mask{
    SUPPORTS,
    SupportType::REFERENCE_DATA,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

template <typename T>
static T combine(T date_part, T time_part) {
  return date_part < T::max() ? date_part + time_part : T::max();
}

template <typename T>
static void validate(const T &value) {
  switch (value.md_update_action) {
    case core::fix::MDUpdateAction::UNKNOWN:
      break;
    case core::fix::MDUpdateAction::NEW:
    case core::fix::MDUpdateAction::CHANGE:
      assert(utils::compare(value.md_entry_size, 0.0) > 0);
      break;
    case core::fix::MDUpdateAction::DELETE:
      assert(utils::compare(value.md_entry_size, 0.0) == 0);
      break;
    case core::fix::MDUpdateAction::DELETE_THRU:
    case core::fix::MDUpdateAction::DELETE_FROM:
      log::fatal("MDUpdateAction not supported: {}"_fmt, value);
      break;
  }
}

template <typename T>
void emplace(MBPUpdate &result, const T &value) {
  new (&result) MBPUpdate{
      .price = value.md_entry_px,
      .quantity = value.md_entry_size,
  };
}

template <typename T>
void emplace(Trade &result, const T &value) {
  new (&result) Trade{
      .side = core::fix::map(value.side),
      .price = value.md_entry_px,
      .quantity = value.md_entry_size,
      .trade_id = value.deribit_trade_id,
  };
}
}  // namespace

MarketData::MarketData(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared,
    bool master)
    : handler_(handler), stream_id_(stream_id), name_(roq::format("{}:{}"_fmt, stream_id_, NAME)),
      master_(master), connection_factory_(context, Flags::fix_uri()),
      connection_(*this, connection_factory_), encode_buffer_(Flags::encode_buffer_size()),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"_sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"_sv),
          .security_list = create_metrics(name_, "security_list"_sv),
          .security_status = create_metrics(name_, "security_status"_sv),
          .market_data_incremental_refresh =
              create_metrics(name_, "market_data_incremental_refresh"_sv),
          .market_data_request_reject = create_metrics(name_, "market_data_request_reject"_sv),
          .market_data_snapshot_full_refresh =
              create_metrics(name_, "market_data_snapshot_full_refresh"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
      },
      security_(security), shared_(shared),
      download_(Flags::fix_request_timeout(), [this](auto state) { return download(state); }) {
}

void MarketData::operator()(const Event<Start> &) {
  connection_.start();
}

void MarketData::operator()(const Event<Stop> &) {
  connection_.stop();
}

void MarketData::operator()(const Event<Timer> &event) {
  if (!connection_.refresh(event.value.now))
    return;
  if (status_ == ConnectionStatus::READY && next_heartbeat_ <= event.value.now) {
    assert(Flags::fix_ping_freq().count() > 0);
    next_heartbeat_ = event.value.now + Flags::fix_ping_freq();
    send_test_request(core::get_system_clock());
  }
}

void MarketData::operator()(const core::net::Manager::Connected &) {
  send_logon();
  (*this)(ConnectionStatus::LOGIN_SENT);
}

void MarketData::operator()(const core::net::Manager::Disconnected &) {
  ++counter_.disconnect;
  outbound_ = {};
  inbound_ = {};
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void MarketData::operator()(const core::net::Manager::Read &read) {
  auto length = read.buffer.length();
  if (length == 0)
    return;
  auto buffer = read.buffer.pullup(length);
  decltype(length) total = 0;
  for (;;) {
    // core::print_memory(buffer, length);  // DEBUG
    auto bytes = core::fix::Reader<FIX_VERSION>::dispatch(
        [&](const core::fix::message_t &message) {
          try {
            check(message.header);
            parse(message);
          } catch (std::exception &) {
            core::print_memory(buffer, length);
            core::print_string_with_escapes(buffer, length);
            throw;
          }
        },
        buffer,
        length);
    assert(bytes <= length);
    if (bytes == 0)
      break;
    total += bytes;
    buffer += bytes;
    length -= bytes;
    if (Flags::fix_debug())
      core::print_string_with_escapes(buffer, bytes);  // DEBUG
  }
  if (total)
    read.buffer.drain(total);
}

void MarketData::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    server::TraceInfo trace_info;
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = (master_ ? SUPPORTS_MASTER : SUPPORTS).get(),
        .status = status_,
        .type = StreamType::FIX,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"_fmt, stream_status);
    server::create_trace_and_dispatch(trace_info, stream_status, handler_);
  }
}

void MarketData::send_logon() {
  auto heart_bt_int =
      std::chrono::duration_cast<std::chrono::seconds>(Flags::fix_ping_freq()).count();
  auto sending_time = core::get_realtime_clock();
  auto raw_data = security_.create_raw_data(
      std::chrono::duration_cast<std::chrono::milliseconds>(sending_time));
  auto password = security_.create_password(raw_data);
  auto cancel_on_disconnect = Flags::fix_cancel_on_disconnect();
  fix::Logon logon{
      .heart_bt_int = utils::safe_cast(heart_bt_int),
      .raw_data_length = utils::safe_cast(raw_data.length()),
      .raw_data = raw_data,
      .username = security_.get_access_key(),
      .password = password,
      .use_wordsafe_tags = false,
      .cancel_on_disconnect = cancel_on_disconnect,
      .deribit_app_id = {},
      .deribit_app_sig = {},
      .deribit_sequential = false,
      .unsubscribe_execution_reports = true,
  };
  send(logon);
}

void MarketData::send_logout(const std::string_view &text) {
  fix::Logout logout{
      .text = text,
  };
  send(logout);
}

void MarketData::send_heartbeat(const std::string_view &test_req_id) {
  fix::Heartbeat heartbeat{
      .test_req_id = test_req_id,
  };
  send(heartbeat);
}

void MarketData::send_test_request(std::chrono::nanoseconds now) {
  // request_id is current time
  stack_buffer_.clear();
  core::charconv::to_string(std::back_inserter(stack_buffer_), now.count());
  auto request_id = std::string_view(stack_buffer_.data(), stack_buffer_.size());
  fix::TestRequest test_request{
      .test_req_id = request_id,
  };
  send(test_request);
}

uint32_t MarketData::download(MarketDataState state) {
  switch (state) {
    case MarketDataState::UNDEFINED:
      assert(false);
      break;
    case MarketDataState::SECURITIES:
      if (master_) {
        download_securities();
        return 1;
      } else {
        return {};
      }
    case MarketDataState::SUBSCRIBE:
      subscribe(symbols_);
      return {};
    case MarketDataState::DONE:
      assert(!ready_);
      ready_ = true;
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

void MarketData::download_securities() {
  auto request_id = shared_.next_request_id();
  fix::SecurityListRequest security_list_request{
      .security_req_id = request_id,
      .security_list_request_type = core::fix::SecurityListRequestType::ALL_SECURITIES,
  };
  send(security_list_request);
}

void MarketData::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.security_list, metrics::PROFILE)
      .write(profile_.security_status, metrics::PROFILE)
      .write(profile_.market_data_incremental_refresh, metrics::PROFILE)
      .write(profile_.market_data_request_reject, metrics::PROFILE)
      .write(profile_.market_data_snapshot_full_refresh, metrics::PROFILE)
      .write(latency_.ping, metrics::LATENCY);
}

void MarketData::update_subscriptions(std::vector<std::string> &symbols) {
  assert(&symbols != &symbols_);
  auto max_size = Flags::fix_market_data_max_subscriptions_per_stream();
  auto offset = symbols_.size();
  if (max_size <= offset)
    return;
  if (symbols.empty())
    return;
  symbols_.reserve(max_size);
  auto length = std::min(max_size - offset, symbols.size());
  assert(length > 0);
  for (size_t i = {}; i < length; ++i) {
    symbols_.emplace_back(symbols.back());
    symbols.pop_back();
  }
  assert(length == (symbols_.size() - offset));
  if (ready_)
    subscribe({&symbols_[offset], length});
}

void MarketData::subscribe(const roq::span<std::string> &symbols) {
  log::info("Subscribe market data"_sv);
  assert(!symbols.empty());
  auto market_depth = Flags::fix_market_data_market_depth();
  auto md_update_type = market_depth ? core::fix::MDUpdateType::INCREMENTAL_REFRESH
                                     : core::fix::MDUpdateType::FULL_REFRESH;
  fix::MDReq md_entry_types[] = {
      {.md_entry_type = core::fix::MDEntryType::BID},
      {.md_entry_type = core::fix::MDEntryType::OFFER},
      {.md_entry_type = core::fix::MDEntryType::TRADE},
  };
  // deribit has acknowledged a limit on # of symbols per request
  auto max_size = Flags::fix_market_data_request_max_size()
                      ? Flags::fix_market_data_request_max_size()
                      : symbols.size();
  std::vector<fix::InstrmtMDReq> related_sym(max_size);
  for (size_t offset = {};; offset += max_size) {
    if (symbols.size() <= offset)
      break;
    auto length = std::min<size_t>(symbols.size() - offset, max_size);
    assert(length > 0);
    for (size_t i = {}; i < length; ++i)
      new (&related_sym[i]) fix::InstrmtMDReq{
          .symbol = symbols[offset + i],
      };
    auto request_id = shared_.next_request_id();
    fix::MarketDataRequest market_data_request{
        .md_req_id = request_id,
        .subscription_request_type = core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
        .market_depth = market_depth,
        .md_update_type = md_update_type,
        .deribit_trade_amount = {},     // 0=none
        .deribit_since_timestamp = {},  // 0=none
        .no_md_entry_types = md_entry_types,
        .no_related_sym = {related_sym.data(), length},
    };
    send(market_data_request);
  }
}

void MarketData::parse(const core::fix::message_t &message) {
  profile_.parse([&]() {
    try {
      parse_helper(message);
    } catch (...) {
      core::tools::UnhandledException::terminate();
    }
  });
}

void MarketData::parse_helper(const core::fix::message_t &message) {
  server::TraceInfo trace_info;
  core::fix::Buffer buffer(decode_buffer_);
  switch (message.header.msg_type) {
    // session
    case core::fix::MsgType::HEARTBEAT: {
      auto heartbeat = fix::Heartbeat::create(message);
      (*this)(message.header, heartbeat, trace_info);
      break;
    }
    case core::fix::MsgType::LOGON: {
      auto logon = fix::Logon::create(message);
      (*this)(message.header, logon, trace_info);
      break;
    }
    case core::fix::MsgType::LOGOUT: {
      auto logout = fix::Logout::create(message);
      (*this)(message.header, logout, trace_info);
      break;
    }
    case core::fix::MsgType::RESEND_REQUEST: {
      auto resend_request = fix::ResendRequest::create(message);
      (*this)(message.header, resend_request, trace_info);
      break;
    }
    case core::fix::MsgType::TEST_REQUEST: {
      auto test_request = fix::TestRequest::create(message);
      (*this)(message.header, test_request, trace_info);
      break;
    }
    // ...
    case core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH: {
      profile_.market_data_incremental_refresh([&]() {
        auto market_data_incremental_referesh =
            fix::MarketDataIncrementalRefresh::create(message, buffer);
        (*this)(message.header, market_data_incremental_referesh, trace_info);
      });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_REQUEST_REJECT: {
      profile_.market_data_request_reject([&]() {
        auto market_data_request_reject = fix::MarketDataRequestReject::create(message);
        (*this)(message.header, market_data_request_reject, trace_info);
      });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
      profile_.market_data_snapshot_full_refresh([&]() {
        auto market_data_snapshot_full_refresh =
            fix::MarketDataSnapshotFullRefresh::create(message, buffer);
        (*this)(message.header, market_data_snapshot_full_refresh, trace_info);
      });
      break;
    }
    case core::fix::MsgType::SECURITY_LIST: {
      profile_.security_list([&]() {
        auto security_list = fix::SecurityList::create(message, buffer);
        (*this)(message.header, security_list, trace_info);
      });
      break;
    }
    case core::fix::MsgType::SECURITY_STATUS: {
      profile_.security_status([&]() {
        auto security_status = fix::SecurityStatus::create(message, buffer);
        (*this)(message.header, security_status, trace_info);
      });
      break;
    }
    default:
      log::warn("Unexpected msg_type={}"_fmt, message.header.msg_type);
      break;
  }
}

void MarketData::operator()(
    const core::fix::header_t &header, const fix::Heartbeat &heartbeat, const server::TraceInfo &) {
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  log::trace_3("event(header={}, heartbeat={})"_fmt, header, heartbeat);
  if (!heartbeat.test_req_id.empty()) {
    auto send_time = core::from_chars<uint64_t>(heartbeat.test_req_id);
    auto latency =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - decltype(now){send_time}) /
        2;  // 1-way
    server::TraceInfo trace_info;
    ExternalLatency external_latency{
        .stream_id = stream_id_,
        .latency = latency,
    };
    server::create_trace_and_dispatch(trace_info, external_latency, handler_);
    latency_.ping.update(latency);
  }
}

void MarketData::operator()(
    const core::fix::header_t &header, const fix::Logon &logon, const server::TraceInfo &) {
  log::trace_1("event(header={}, logon={})"_fmt, header, logon);
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void MarketData::operator()(
    const core::fix::header_t &header, const fix::Logout &logout, const server::TraceInfo &) {
  log::warn("event(header={}, logout={})"_fmt, header, logout);
  (*this)(ConnectionStatus::LOGGED_OUT);
  ready_ = false;
  // note! mandated, must send a logout response
  send_logout(LOGOUT_RESPONSE);
  log::info("closing connection"_sv);
  connection_.close();
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::ResendRequest &resend_request,
    const server::TraceInfo &) {
  log::warn("event(header={}, resend_request={})"_fmt, header, resend_request);
  log::info("closing connection"_sv);
  connection_.close();
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::TestRequest &test_request,
    const server::TraceInfo &) {
  log::trace_1("event(header={}, test_request={})"_fmt, header, test_request);
  send_heartbeat(test_request.test_req_id);
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::SecurityList &security_list,
    const server::TraceInfo &trace_info) {
  log::trace_2("event(header={}, security_list={})"_fmt, header, security_list);
  if (security_list.no_related_sym.size() > 0) {
    size_t counter = {};
    std::vector<std::string> symbols;
    symbols.reserve(security_list.no_related_sym.size());
    for (auto &instrument : security_list.no_related_sym) {
      log::trace_1("instrument={}"_fmt, instrument);
      auto &symbol = instrument.symbol;
      if (shared_.discard_symbol(symbol))
        continue;
      if (all_symbols_.emplace(symbol).second)  // only include new
        symbols.emplace_back(symbol);
      auto expiry_datetime = combine(
          instrument.maturity_date,
          core::charconv::time_from_string<std::chrono::milliseconds>(instrument.maturity_time));
      auto expiry_datetime_utc = expiry_datetime;
      ReferenceData reference_data{
          .stream_id = stream_id_,
          .exchange = Flags::exchange(),
          .symbol = symbol,
          .description = instrument.security_desc,
          .security_type = fix::map_security_type(instrument.security_type),
          .currency = instrument.currency,
          .settlement_currency = instrument.settl_currency,
          .commission_currency = instrument.comm_currency,
          .tick_size = instrument.min_price_increment,
          .multiplier = instrument.contract_multiplier,
          .min_trade_vol = instrument.min_trade_vol,
          .option_type = core::fix::map(instrument.put_or_call),
          .strike_currency = instrument.strike_currency,
          .strike_price = instrument.strike_price,
          .underlying = instrument.underlying_symbol,
          .time_zone = {},
          .issue_date = utils::safe_cast(instrument.issue_date),
          .settlement_date = {},
          .expiry_datetime = utils::safe_cast(expiry_datetime),
          .expiry_datetime_utc = utils::safe_cast(expiry_datetime_utc),
      };
      server::create_trace_and_dispatch(trace_info, reference_data, handler_, true);
      ++counter;
    }
    log::info("- securities: {} (/{})"_fmt, counter, security_list.no_related_sym.size());
    if (!symbols.empty()) {
      SymbolsUpdate symbols_update{
          .symbols = symbols,
      };
      handler_(symbols_update);
    }
  }
  download_.check(MarketDataState::SECURITIES);
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::SecurityStatus &security_status,
    const server::TraceInfo &) {
  log::trace_2("event(header={}, security_status={})"_fmt, header, security_status);
  // XXX should we use it or not?
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::MarketDataIncrementalRefresh &market_data_incremental_refresh,
    const server::TraceInfo &trace_info) {
  log::trace_3(
      "event(header={}, market_data_incremental_refresh={})"_fmt,
      header,
      market_data_incremental_refresh);

  core::back_emplacer bids(shared_.bids), asks(shared_.asks);
  core::back_emplacer trades(shared_.trades);
  core::back_emplacer statistics(shared_.statistics);

  // open interest
  statistics.emplace_back([&](auto &result) {
    new (&result) Statistics{
        .type = StatisticsType::PRE_OPEN_INTEREST,
        .value = market_data_incremental_refresh.open_interest,
        .begin_time_utc = {},
        .end_time_utc = {},
    };
  });
  // mark price
  statistics.emplace_back([&](auto &result) {
    new (&result) Statistics{
        .type = StatisticsType::PRE_SETTLEMENT_PRICE,
        .value = market_data_incremental_refresh.mark_price,
        .begin_time_utc = {},
        .end_time_utc = {},
    };
  });
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : market_data_incremental_refresh.no_md_entries) {
    if (exchange_time_utc < item.md_entry_date)
      exchange_time_utc = item.md_entry_date;
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        validate(item);
        bids.emplace_back([&item](auto &result) { emplace(result, item); });
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        validate(item);
        asks.emplace_back([&item](auto &result) { emplace(result, item); });
        break;
      }
      case core::fix::MDEntryType::TRADE: {
        trades.emplace_back([&item](auto &result) { emplace(result, item); });
        break;
      }
      case core::fix::MDEntryType::INDEX_VALUE:
        statistics.emplace_back([&](auto &result) {
          new (&result) Statistics{
              .type = StatisticsType::INDEX_VALUE,
              .value = item.md_entry_px,
              .begin_time_utc = {},
              .end_time_utc = {},
          };
        });
        break;
      case core::fix::MDEntryType::SETTLEMENT_PRICE:
        statistics.emplace_back([&](auto &result) {
          new (&result) Statistics{
              .type = StatisticsType::SETTLEMENT_PRICE,
              .value = item.md_entry_px,
              .begin_time_utc = {},
              .end_time_utc = {},
          };
        });
        break;
      default:
        log::warn("unsupported: {}"_fmt, item);
        break;
    }
  }
  if (!(bids.empty() && asks.empty())) {
    MarketByPriceUpdate market_by_price_update{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = market_data_incremental_refresh.symbol,
        .bids = bids,
        .asks = asks,
        .snapshot = false,  // incremental
        .exchange_time_utc = exchange_time_utc,
    };
    auto is_last = statistics.empty() && trades.empty();
    server::create_trace_and_dispatch(trace_info, market_by_price_update, handler_, is_last);
  }
  if (!trades.empty()) {
    TradeSummary trade_summary{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = market_data_incremental_refresh.symbol,
        .trades = trades,
        .exchange_time_utc = exchange_time_utc,
    };
    auto is_last = statistics.empty();
    server::create_trace_and_dispatch(trace_info, trade_summary, handler_, is_last);
  }
  if (!statistics.empty()) {
    StatisticsUpdate statistics_update{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = market_data_incremental_refresh.symbol,
        .statistics = statistics,
        .snapshot = false,
        .exchange_time_utc = exchange_time_utc,
    };
    server::create_trace_and_dispatch(trace_info, statistics_update, handler_, true);
  }
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::MarketDataRequestReject &market_data_request_reject,
    const server::TraceInfo &) {
  log::warn(
      "event(header={}, market_data_request_reject={})"_fmt, header, market_data_request_reject);
  log::fatal("Unexpected"_sv);  // don't know how to continue
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::MarketDataSnapshotFullRefresh &market_data_snapshot_full_refresh,
    const server::TraceInfo &trace_info) {
  log::trace_3(
      "event(header={}, market_data_snapshot_full_refresh={})"_fmt,
      header,
      market_data_snapshot_full_refresh);
  core::back_emplacer bids(shared_.bids), asks(shared_.asks);
  core::back_emplacer statistics(shared_.statistics);
  for (auto &item : market_data_snapshot_full_refresh.no_md_entries) {
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        validate(item);
        bids.emplace_back([&item](auto &result) { emplace(result, item); });
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        validate(item);
        asks.emplace_back([&item](auto &result) { emplace(result, item); });
        break;
      }
      case core::fix::MDEntryType::TRADE:
        break;  // drop
      case core::fix::MDEntryType::INDEX_VALUE:
        statistics.emplace_back([&](auto &result) {
          new (&result) Statistics{
              .type = StatisticsType::INDEX_VALUE,
              .value = item.md_entry_px,
              .begin_time_utc = {},
              .end_time_utc = {},
          };
        });
        break;
      case core::fix::MDEntryType::SETTLEMENT_PRICE:
        statistics.emplace_back([&](auto &result) {
          new (&result) Statistics{
              .type = StatisticsType::SETTLEMENT_PRICE,
              .value = item.md_entry_px,
              .begin_time_utc = {},
              .end_time_utc = {},
          };
        });
        break;
      default:
        log::warn("unsupported: {}"_fmt, item);
        break;
    }
  }
  if (!(bids.empty() && asks.empty())) {
    auto is_last = statistics.empty();
    MarketByPriceUpdate market_by_price_update{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = market_data_snapshot_full_refresh.symbol,
        .bids = bids,
        .asks = asks,
        .snapshot = true,
        .exchange_time_utc = {},
    };
    server::create_trace_and_dispatch(trace_info, market_by_price_update, handler_, is_last);
  }
  if (!statistics.empty()) {
    StatisticsUpdate statistics_update{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = market_data_snapshot_full_refresh.symbol,
        .statistics = statistics,
        .snapshot = true,
        .exchange_time_utc = {},
    };
    server::create_trace_and_dispatch(trace_info, statistics_update, handler_, true);
  }
}

// utilities

template <typename T>
void MarketData::send(const T &event) {
  send(event, core::get_realtime_clock());
}

template <typename T>
void MarketData::send(const T &event, std::chrono::nanoseconds sending_time) {
  core::fix::Writer writer(
      encode_buffer_,
      FIX_VERSION,
      T::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      outbound_.msg_seq_num,
      sending_time);
  auto message = event.encode(writer);
  // message.print();  // DEBUG
  connection_.send(message);
}

void MarketData::check(const core::fix::header_t &header) {
  auto current = header.msg_seq_num;
  auto expected = inbound_.msg_seq_num + 1;
  if (ROQ_UNLIKELY(current != expected)) {
    if (expected < current) {
      log::warn(
          "*** SEQUENCE GAP *** "
          "current={} previous={} distance={}"_fmt,
          current,
          inbound_.msg_seq_num,
          current - inbound_.msg_seq_num);
    } else {
      log::warn(
          "*** SEQUENCE REPLAY *** "
          "current={} previous={} distance={}"_fmt,
          current,
          inbound_.msg_seq_num,
          inbound_.msg_seq_num - current);
    }
  }
  inbound_.msg_seq_num = current;
}

}  // namespace deribit
}  // namespace roq
