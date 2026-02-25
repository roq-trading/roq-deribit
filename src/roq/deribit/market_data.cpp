/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/market_data.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"

#include "roq/utils/compare.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/charconv/from_chars.hpp"
#include "roq/utils/charconv/to_string.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/utils/debug/fix/message.hpp"
#include "roq/utils/debug/hex/message.hpp"

#include "roq/fix/map.hpp"
#include "roq/fix/reader.hpp"

#include "roq/deribit/common.hpp"
#include "roq/deribit/utils.hpp"

#include "roq/deribit/fix/utils.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const NAME = "md"sv;

auto const LOGOUT_RESPONSE = "LOGOUT"sv;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto publish_market_by_price(auto &shared) {
  return !shared.has_multicast() || shared.settings.multicast.disable_market_by_price;
}

auto publish_trade_summary(auto &shared) {
  return !shared.has_multicast() || shared.settings.multicast.disable_trade_summary;
}

auto get_supports(auto master, auto publish_market_by_price, auto publish_trade_summary) {
  Mask<SupportType> result;
  if (master) {
    result |= SupportType::REFERENCE_DATA;
  }
  if (publish_market_by_price) {
    result |= SupportType::MARKET_BY_PRICE;
  }
  if (publish_trade_summary) {
    result |= SupportType::TRADE_SUMMARY;
  }
  return result;
}

auto create_connection_factory(auto &settings, auto &context) {
  auto uri = settings.fix.uri;
  auto config = io::net::ConnectionFactory::Config{
      .interface = settings.misc.test_local_interface,
      .uris = {&uri, 1},
      .validate_certificate = settings.net.tls_validate_certificate,
  };
  return io::net::ConnectionFactory::create(context, config);
}

auto create_connection_manager(auto &handler, auto &settings, auto &connection_factory) {
  auto config = io::net::ConnectionManager::Config{
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = settings.net.disconnect_on_idle_timeout,
      .always_reconnect = true,
  };
  return io::net::ConnectionManager::create(handler, connection_factory, config);
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};

// following are used from several places

void validate(auto &value) {
  switch (value.md_update_action) {
    using enum roq::fix::MDUpdateAction;
    case UNDEFINED:
    case UNKNOWN:
      break;
    case NEW:
      // assert(utils::is_greater(value.md_entry_size, 0.0));
      break;
    case CHANGE:
      // assert(utils::is_greater(value.md_entry_size, 0.0));
      break;
    case DELETE:
#ifndef NDEBUG
      if (!utils::is_zero(value.md_entry_size)) {
        log::error("md_entry={}"sv, value);
      }
#endif
      // assert(utils::is_zero(value.md_entry_size));
      break;
    case DELETE_THRU:
    case DELETE_FROM:
    case OVERLAY:
      log::fatal("MDUpdateAction not supported: {}"sv, value);
      break;
  }
}

template <typename T>
void emplace_back(T &result, auto &value) {
  using value_type = typename T::value_type;
  if constexpr (std::is_same_v<value_type, MBPUpdate>) {
    auto mbp_update = MBPUpdate{
        .price = value.md_entry_px,
        .quantity = value.md_entry_size,
        .implied_quantity = NaN,
        .number_of_orders = {},
        .update_action = {},
        .price_level = {},
    };
    result.emplace_back(std::move(mbp_update));
  } else if constexpr (std::is_same_v<value_type, Trade>) {
    auto trade = Trade{
        .side = map(value.side),
        .price = value.md_entry_px,
        .quantity = value.md_entry_size,
        .trade_id = value.deribit_trade_id,
        .taker_order_id = value.order_id,
        .maker_order_id = value.secondary_order_id,
    };
    result.emplace_back(std::move(trade));
  }
}
}  // namespace

// === IMPLEMENTATION ===

MarketData::MarketData(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared, size_t index, bool master)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, master_{master},
      publish_market_by_price_{publish_market_by_price(shared)}, publish_trade_summary_{publish_trade_summary(shared)},
      supports_{get_supports(master_, publish_market_by_price_, publish_trade_summary_)}, exchange_{shared.settings.exchange},
      fix_debug_{shared.settings.fix.debug}, fix_request_timeout_{shared.settings.fix.request_timeout}, fix_ping_freq_{shared.settings.fix.ping_freq},
      connection_factory_{create_connection_factory(shared.settings, context)},
      connection_manager_{create_connection_manager(*this, shared.settings, *connection_factory_)}, encode_buffer_(shared.settings.misc.encode_buffer_size),
      decode_buffer_(shared.settings.misc.decode_buffer_size),
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .security_list = create_metrics(shared.settings, name_, "security_list"sv),
          .security_status = create_metrics(shared.settings, name_, "security_status"sv),
          .market_data_incremental_refresh = create_metrics(shared.settings, name_, "market_data_incremental_refresh"sv),
          .market_data_request_reject = create_metrics(shared.settings, name_, "market_data_request_reject"sv),
          .market_data_snapshot_full_refresh = create_metrics(shared.settings, name_, "market_data_snapshot_full_refresh"sv),
          .market_data_request = create_metrics(shared.settings, name_, "market_data_request"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      account_{account}, shared_{shared}, download_{fix_request_timeout_, [this](auto state) { return download(state); }} {
}

void MarketData::operator()(Event<Start> const &) {
  (*connection_manager_).start();
}

void MarketData::operator()(Event<Stop> const &) {
  (*connection_manager_).stop();
}

void MarketData::operator()(Event<Timer> const &event) {
  if (!(*connection_manager_).refresh(event.value.now)) {
    return;
  }
  if (last_logon_or_heartbeat_.count() != 0 && fix_request_timeout_.count() != 0 && (event.value.now - last_logon_or_heartbeat_) > fix_request_timeout_) {
    log::warn("*** DETECTED TIMEOUT ***"sv);
    log::info("closing connection"sv);
    (*connection_manager_).close();
  } else {
    if (connection_status_ == ConnectionStatus::READY) {
      if (test_disconnect_time_.count() != 0 && test_disconnect_time_ < event.value.now) [[unlikely]] {
        if (shared_.settings.fix.test_market_data_disconnect.count() != 0) {
          log::warn("*** TEST: DISCONNECT (stream_id={}) ***"sv, stream_id_);
          log::info("closing connection"sv);
          (*connection_manager_).close();
        }
      } else {
        if (next_heartbeat_ <= event.value.now) {
          assert(fix_ping_freq_.count() > 0);
          next_heartbeat_ = event.value.now + fix_ping_freq_;
          send_test_request(clock::get_system());
        }
      }
    }
  }
}

// io::net::ConnectionManager::Handler

void MarketData::operator()(io::net::ConnectionManager::Connected const &) {
  send_logon();
  (*this)(ConnectionStatus::LOGIN_SENT);
}

void MarketData::operator()(io::net::ConnectionManager::Disconnected const &) {
  ++counter_.disconnect;
  outbound_ = {};
  inbound_ = {};
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
  // test
  test_disconnect_time_ = {};
}

void MarketData::operator()(io::net::ConnectionManager::Read const &) {
  TraceInfo trace_info;
  auto parse_message = [&](auto &message) {
    check(message.header);
    Trace event{trace_info, message};
    parse(event);
  };
  auto log_message = [this](auto &message) {
    if (fix_debug_) [[unlikely]] {
      std::string_view tmp{reinterpret_cast<char const *>(std::data(message)), std::size(message)};
      auto market_data = tmp.find("\00135=X\001"sv) != std::string_view::npos || tmp.find("\00135=W\001"sv) != std::string_view::npos;
      if (market_data) {
        log::info<2>("{}"sv, utils::debug::fix::Message{message});
      } else {
        log::info("{}"sv, utils::debug::fix::Message{message});
      }
    }
  };
  auto buffer = (*connection_manager_).buffer();
  try {
    size_t total_bytes = 0;
    while (!std::empty(buffer)) {
      auto bytes = roq::fix::Reader<FIX_VERSION>::dispatch(buffer, parse_message, log_message);
      if (bytes == 0) {
        break;
      }
      assert(bytes <= std::size(buffer));
      total_bytes += bytes;
      buffer = buffer.subspan(bytes);
    }
    (*connection_manager_).drain(total_bytes);
  } catch (std::exception &) {
    log::warn("{}"sv, utils::debug::fix::Message{buffer});
#ifndef NDEBUG
    log::warn("{}"sv, utils::debug::hex::Message{buffer});
#endif
    throw;
  }
}

void MarketData::operator()(io::net::ConnectionManager::Write const &) {
}

void MarketData::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = {},
      .supports = supports_,
      .transport = Transport::TCP,
      .protocol = Protocol::FIX,
      .encoding = {Encoding::FIX},
      .priority = Priority::PRIMARY,
      .connection_status = connection_status_,
      .reason = reason,
      .interface = (*connection_factory_).get_interface(),
      .authority = (*connection_factory_).get_current_authority(),
      .path = (*connection_factory_).get_current_path(),
      .proxy = {},
  };
  log::info<1>("stream_status={}"sv, stream_status);
  create_trace_and_dispatch(handler_, trace_info, stream_status);
}

void MarketData::send_logon() {
  auto heart_bt_int = std::chrono::duration_cast<std::chrono::seconds>(fix_ping_freq_).count();
  auto now = clock::get_realtime<std::chrono::milliseconds>();
  auto raw_data = account_.create_raw_data(now);
  auto password = account_.create_password(raw_data);
  auto cancel_on_disconnect = shared_.settings.fix.cancel_on_disconnect;
  auto logon = fix::Logon{
      .heart_bt_int = utils::safe_cast(heart_bt_int),
      .raw_data_length = utils::safe_cast(std::size(raw_data)),
      .raw_data = raw_data,
      .username = account_.key,
      .password = password,
      .use_wordsafe_tags = false,
      .cancel_on_disconnect = cancel_on_disconnect,
      .deribit_app_id = {},
      .deribit_app_sig = {},
      .deribit_sequential = false,
      .unsubscribe_execution_reports = true,
  };
  send(logon);
  last_logon_or_heartbeat_ = clock::get_system();
}

void MarketData::send_logout(std::string_view const &text) {
  auto logout = fix::Logout{
      .text = text,
  };
  send(logout);
}

void MarketData::send_heartbeat(std::string_view const &test_req_id) {
  auto heartbeat = fix::Heartbeat{
      .test_req_id = test_req_id,
  };
  send(heartbeat);
}

void MarketData::send_test_request(std::chrono::nanoseconds now) {
  // request_id is current time
  request_id_.clear();
  utils::charconv::to_string(std::back_inserter(request_id_), now.count());
  auto test_request = fix::TestRequest{
      .test_req_id = request_id_,
  };
  send(test_request);
  if (last_logon_or_heartbeat_.count() == 0) {
    last_logon_or_heartbeat_ = now;
  }
}

uint32_t MarketData::download(MarketDataState state) {
  switch (state) {
    using enum MarketDataState;
    case UNDEFINED:
      assert(false);
      break;
    case SECURITIES:
      if (master_) {
        (*this)(ConnectionStatus::DOWNLOADING, "securities"sv);
        download_securities();
        return 1;
      } else {
        return 0;
      }
    case SUBSCRIBE:
      assert(!ready_);
      ready_ = true;
      subscribe();
      return 0;
    case DONE: {
      (*this)(ConnectionStatus::READY);
      // test
      auto now = clock::get_system();
      if (shared_.settings.fix.test_market_data_disconnect.count() != 0) {
        test_disconnect_time_ = now + shared_.settings.fix.test_market_data_disconnect;
        log::warn(
            "*** TEST: DISCONNECT IN {} (stream_id={}) ***"sv,
            std::chrono::duration_cast<std::chrono::seconds>(shared_.settings.fix.test_market_data_disconnect),
            stream_id_);
      }
      return 0;
    }
  }
  assert(false);
  return 0;
}

void MarketData::download_securities() {
  auto request_id = shared_.next_request_id();
  auto security_list_request = fix::SecurityListRequest{
      .security_req_id = request_id,
      .security_list_request_type = roq::fix::SecurityListRequestType::ALL_SECURITIES,
      .subscription_request_type = roq::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
  };
  send(security_list_request);
}

void MarketData::operator()(metrics::Writer &writer) const {
  writer  //
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.security_list, metrics::Type::PROFILE)
      .write(profile_.security_status, metrics::Type::PROFILE)
      .write(profile_.market_data_incremental_refresh, metrics::Type::PROFILE)
      .write(profile_.market_data_request_reject, metrics::Type::PROFILE)
      .write(profile_.market_data_snapshot_full_refresh, metrics::Type::PROFILE)
      .write(profile_.market_data_request, metrics::Type::PROFILE)
      .write(latency_.ping, metrics::Type::LATENCY);
}

void MarketData::subscribe(size_t start_from) {
  if (ready()) {
    subscribe(shared_.symbols.get_slice(index_, start_from));
  }
}

void MarketData::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols)) {
    return;
  }
  log::info("Subscribe symbols=[{}]"sv, fmt::join(symbols, ","sv));
  auto market_depth = shared_.settings.fix.market_data_market_depth;
  auto md_update_type = market_depth != 0 ? roq::fix::MDUpdateType::INCREMENTAL_REFRESH : roq::fix::MDUpdateType::FULL_REFRESH;
  std::array<fix::MDReq, 3> md_entry_types{{
      {.md_entry_type = roq::fix::MDEntryType::BID},
      {.md_entry_type = roq::fix::MDEntryType::OFFER},
      {.md_entry_type = roq::fix::MDEntryType::TRADE},
  }};
  // deribit has acknowledged a limit on # of symbols per request
  auto max_size = shared_.settings.fix.market_data_request_max_size != 0 ? shared_.settings.fix.market_data_request_max_size : std::size(symbols);
  std::vector<fix::InstrmtMDReq> related_sym(max_size);
  for (size_t offset = 0;; offset += max_size) {
    if (std::size(symbols) <= offset) {
      break;
    }
    auto length = std::min<size_t>(std::size(symbols) - offset, max_size);
    assert(length > 0);
    for (size_t i = 0; i < length; ++i) {
      new (&related_sym[i]) fix::InstrmtMDReq{
          .symbol = symbols[offset + i],
      };
    }
    auto request_id = shared_.next_request_id();
    auto market_data_request = fix::MarketDataRequest{
        .md_req_id = request_id,
        .subscription_request_type = roq::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
        .market_depth = market_depth,
        .md_update_type = md_update_type,
        .deribit_trade_amount = {},     // 0=none
        .deribit_since_timestamp = {},  // 0=none
        .no_md_entry_types = md_entry_types,
        .no_related_sym = {std::data(related_sym), length},
    };
    send(market_data_request);
  }
}

void MarketData::unsubscribe(std::span<Symbol const> const &symbols) {
  log::info("Unsubscribe market data"sv);
  assert(!std::empty(symbols));
  std::array<fix::MDReq, 3> md_entry_types{{
      {.md_entry_type = roq::fix::MDEntryType::BID},
      {.md_entry_type = roq::fix::MDEntryType::OFFER},
      {.md_entry_type = roq::fix::MDEntryType::TRADE},
  }};
  // deribit has acknowledged a limit on # of symbols per request
  auto max_size = shared_.settings.fix.market_data_request_max_size != 0 ? shared_.settings.fix.market_data_request_max_size : std::size(symbols);
  std::vector<fix::InstrmtMDReq> related_sym(max_size);
  for (size_t offset = 0;; offset += max_size) {
    if (std::size(symbols) <= offset) {
      break;
    }
    auto length = std::min<size_t>(std::size(symbols) - offset, max_size);
    assert(length > 0);
    for (size_t i = 0; i < length; ++i) {
      new (&related_sym[i]) fix::InstrmtMDReq{
          .symbol = symbols[offset + i],
      };
    }
    auto request_id = shared_.next_request_id();
    auto market_data_request = fix::MarketDataRequest{
        .md_req_id = request_id,
        .subscription_request_type = roq::fix::SubscriptionRequestType::UNSUBSCRIBE,
        .market_depth = {},
        .md_update_type = {},
        .deribit_trade_amount = {},
        .deribit_since_timestamp = {},
        .no_md_entry_types = md_entry_types,
        .no_related_sym = {std::data(related_sym), length},
    };
    send(market_data_request);
  }
}

void MarketData::resubscribe(std::string_view const &symbol) {
  log::debug(R"(*** RESUBSCRIBE *** (symbol="{}"))"sv, symbol);
  log::warn<1>(R"(*** RESUBSCRIBE *** (symbol="{}"))"sv, symbol);
  if (latch_.find(symbol) != std::end(latch_)) {
    return;
  }
  log::info<1>(R"(Latch symbol="{}")"sv, symbol);
  latch_.emplace(symbol);
  Symbol tmp{symbol};  // copy
  std::span symbols{&tmp, 1};
  unsubscribe(symbols);
  subscribe(symbols);
}

void MarketData::parse(Trace<roq::fix::Message> const &event) {
  profile_.parse([&]() { parse_helper(event); });
}

void MarketData::parse_helper(Trace<roq::fix::Message> const &event) {
  auto &trace_info = event.trace_info;
  auto &message = event.value;
  switch (message.header.msg_type) {
    using enum roq::fix::MsgType;
    // session
    case HEARTBEAT: {
      auto heartbeat = fix::Heartbeat::create(message);
      create_trace_and_dispatch(*this, trace_info, heartbeat, message.header);
      break;
    }
    case LOGON: {
      auto logon = fix::Logon::create(message);
      create_trace_and_dispatch(*this, trace_info, logon, message.header);
      break;
    }
    case LOGOUT: {
      auto logout = fix::Logout::create(message);
      create_trace_and_dispatch(*this, trace_info, logout, message.header);
      break;
    }
    case RESEND_REQUEST: {
      auto resend_request = fix::ResendRequest::create(message);
      create_trace_and_dispatch(*this, trace_info, resend_request, message.header);
      break;
    }
    case TEST_REQUEST: {
      auto test_request = fix::TestRequest::create(message);
      create_trace_and_dispatch(*this, trace_info, test_request, message.header);
      break;
    }
    // ...
    case MARKET_DATA_INCREMENTAL_REFRESH:
      profile_.market_data_incremental_refresh([&]() {
        auto market_data_incremental_refresh = fix::MarketDataIncrementalRefresh::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, market_data_incremental_refresh, message.header);
      });
      break;
    case MARKET_DATA_REQUEST_REJECT:
      profile_.market_data_request_reject([&]() {
        auto market_data_request_reject = fix::MarketDataRequestReject::create(message);
        create_trace_and_dispatch(*this, trace_info, market_data_request_reject, message.header);
      });
      break;
    case MARKET_DATA_SNAPSHOT_FULL_REFRESH:
      profile_.market_data_snapshot_full_refresh([&]() {
        auto market_data_snapshot_full_refresh = fix::MarketDataSnapshotFullRefresh::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, market_data_snapshot_full_refresh, message.header);
      });
      break;
    case SECURITY_LIST:
      profile_.security_list([&]() {
        auto security_list = fix::SecurityList::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, security_list, message.header);
      });
      break;
    case SECURITY_STATUS:
      profile_.security_status([&]() {
        auto security_status = fix::SecurityStatus::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, security_status, message.header);
      });
      break;
    // weird
    case MARKET_DATA_REQUEST:
      profile_.market_data_request([&]() {
        // XXX HANS why do we get this message?
      });
      break;
    default:
      log::warn("Unexpected msg_type={}"sv, message.header.msg_type);
  }
}

void MarketData::operator()(Trace<fix::Heartbeat> const &event, roq::fix::Header const &header) {
  auto now = clock::get_system();
  auto &[trace_info, heartbeat] = event;
  log::info<3>("event={{header={}, heartbeat={}}}"sv, header, heartbeat);
  last_logon_or_heartbeat_ = {};
  if (!std::empty(heartbeat.test_req_id)) {
    auto send_time = std::chrono::nanoseconds{utils::charconv::from_chars<uint64_t>(heartbeat.test_req_id)};
    auto latency = (now - send_time) / 2;  // 1-way
    auto external_latency = ExternalLatency{
        .stream_id = stream_id_,
        .account = {},
        .latency = latency,
    };
    create_trace_and_dispatch(handler_, trace_info, external_latency);
    latency_.ping.update(latency);
  }
}

void MarketData::operator()(Trace<fix::Logon> const &event, roq::fix::Header const &header) {
  auto &[trace_info, logon] = event;
  log::info<2>("event={{header={}, logon={}}}"sv, header, logon);
  download_.begin();
}

void MarketData::operator()(Trace<fix::Logout> const &event, roq::fix::Header const &header) {
  auto &[trace_info, logout] = event;
  log::warn("event={{header={}, logout={}}}"sv, header, logout);
  (*this)(ConnectionStatus::LOGGED_OUT);
  ready_ = false;
  // note! mandated, must send a logout response
  send_logout(LOGOUT_RESPONSE);
  log::info("closing connection"sv);
  (*connection_manager_).close();
}

void MarketData::operator()(Trace<fix::ResendRequest> const &event, roq::fix::Header const &header) {
  auto &[trace_info, resend_request] = event;
  log::warn("event={{header={}, resend_request={}}}"sv, header, resend_request);
  log::info("closing connection"sv);
  (*connection_manager_).close();
}

void MarketData::operator()(Trace<fix::TestRequest> const &event, roq::fix::Header const &header) {
  auto &[trace_info, test_request] = event;
  log::info<1>("event={{header={}, test_request={}}}"sv, header, test_request);
  send_heartbeat(test_request.test_req_id);
}

// Roq: (seems wrong, sometimes)
//   base_currency <= settl_currency
//   quote_currency <= currency
//
// Deribit: (FIX)
//   symbol                ETH_BTC  BTC_USDT  BTC-PERPETUAL  BTC-27DEC24  BTC-27DEC24-62000-P  BTC_USDC-PERPETUAL
//   security_type         FXSPOT   FXSPOT    FUT            FUT          OPT                  FUT
//   strike_currency                                                      USD
//   currency              ETH      BTC       USD (???)      USD (???)    BTC                  USDC
//   settl_currency        BTC      USDT      USD            USD          USD                  USDC
//   comm_currency         ETH_BTC  BTC_USDT  BTC            BTC          BTC                  USDC
//   price_quote_currency  BTC      USDT
//
//                                            inverse        inverse                           linear (??? no BTC ???)
//
//   base                  +ETH     +BTC      -BTC           -BTC         +BTC                 -BTC
//   quote                 +BTC     +USDT     ?USD           ?USD         ?USD                 ?USDC
//   settle                +BTC     +USDT     -BTC           -BTC         -BTC                 +USDC
//
//
//
//                        JSON:                                                                  FIX:
//                        base_currency  quote_currency  settlement_currency  counter_currency   currency  price_quote_currency  settl_currency  comm_currency
//                                                                                               <15>      <1524>                <120>           <479>
//
// BTC_USDT               BTC            USDT                                 USDT               BTC       USDT                  USDT(?)         BTC_USDT(?)
// BTC_USDC-PERPETUAL     BTC            USDC            USDC                 USDC               USDC(?)   (?)                   USDC            USDC
// BTC-PERPETUAL          BTC            USD             BTC                  USD                USD(?)    (?)                   USD             BTC
// BTC-27DEC24-62000-P    BTC            BTC             BTC                  USD                BTC       (?)                   USD             BTC

void MarketData::operator()(Trace<fix::SecurityList> const &event, roq::fix::Header const &header) {
  auto &[trace_info, security_list] = event;
  log::info<2>("event={{header={}, security_list={}}}"sv, header, security_list);
  (*connection_manager_).touch(trace_info.source_receive_time);
  auto combine = []<typename T>(T date_part, T time_part) { return date_part < T::max() ? date_part + time_part : T::max(); };
  if (std::size(security_list.no_related_sym) > 0) {
    size_t counter = 0;
    std::vector<Symbol> symbols;
    symbols.reserve(std::size(security_list.no_related_sym));
    for (auto &item : security_list.no_related_sym) {
      log::info<2>("item={}"sv, item);
      auto &symbol = item.symbol;
      auto discard = shared_.discard_symbol(symbol);
      auto multiplier = compute_contracts_multiplier(item.contract_multiplier);
      if (shared_.settings.misc.use_fix_reference_data) {
        auto security_type = fix::map_security_type(item.security_type);
        auto option_type = map(item.put_or_call);
        auto expiry_time = utils::charconv::from_chars<std::chrono::milliseconds>(item.maturity_time, utils::charconv::Format::TIME);
        auto expiry_datetime = combine(item.maturity_date, expiry_time);
        auto expiry_datetime_utc = expiry_datetime;
        auto reference_data = ReferenceData{
            .stream_id = stream_id_,
            .exchange = exchange_,
            .symbol = symbol,
            .description = item.security_desc,
            .security_type = security_type,
            .external_security_id = {},
            .cfi_code = {},
            .base_currency = item.settl_currency,
            .quote_currency = item.currency,
            .settlement_currency = item.settl_currency,
            .margin_currency = {},
            .commission_currency = item.comm_currency,
            .tick_size = item.min_price_increment,
            .tick_size_steps = {},
            .multiplier = item.contract_multiplier,  // XXX which one is it ???
            .min_notional = NaN,
            .min_trade_vol = item.min_trade_vol,
            .max_trade_vol = NaN,
            .trade_vol_step_size = item.min_trade_vol,
            .option_type = option_type,
            .strike_currency = item.strike_currency,
            .strike_price = item.strike_price,
            .underlying = item.underlying_symbol,
            .time_zone = {},
            .issue_date = utils::safe_cast(item.issue_date),
            .settlement_date = {},
            .expiry_datetime = utils::safe_cast(expiry_datetime),
            .expiry_datetime_utc = utils::safe_cast(expiry_datetime_utc),
            .exchange_time_utc = {},
            .exchange_sequence = {},
            .sending_time_utc = {},
            .discard = discard,
        };
        create_trace_and_dispatch(handler_, trace_info, reference_data, true);
      }
      shared_.multiplier[symbol] = multiplier;
      if (discard) {
        log::info<1>(R"(Drop symbol="{}")"sv, item.symbol);
        continue;
      }
      if (shared_.all_symbols.emplace(symbol).second) {  // only include new
        symbols.emplace_back(symbol);
      }
      ++counter;
    }
    log::info<2>("- securities: {} (/{})"sv, counter, std::size(security_list.no_related_sym));
    if (!std::empty(symbols)) {
      auto symbols_update = SymbolsUpdate{
          .symbols = symbols,
      };
      handler_(symbols_update);
    }
  }
  download_.check_relaxed(MarketDataState::SECURITIES);
}

void MarketData::operator()(Trace<fix::SecurityStatus> const &event, roq::fix::Header const &header) {
  auto &[trace_info, security_status] = event;
  log::info<2>("event={{header={}, security_status={}}}"sv, header, security_status);
  // XXX should we use it or not?
}

void MarketData::operator()(Trace<fix::MarketDataIncrementalRefresh> const &event, roq::fix::Header const &header) {
  auto &trace_info = event.trace_info;
  auto &market_data_incremental_refresh = event.value;
  log::info<3>("event={{header={}, market_data_incremental_refresh={}}}"sv, header, market_data_incremental_refresh);
  (*connection_manager_).touch(trace_info.source_receive_time);
  auto symbol = market_data_incremental_refresh.symbol;
  auto &mbp = shared_.get_mbp();
  auto &trades = shared_.get_trades();
  auto &statistics = shared_.get_statistics();
  // open interest
  {
    auto statistics_2 = Statistics{
        .type = StatisticsType::PRE_OPEN_INTEREST,
        .value = market_data_incremental_refresh.open_interest,
        .begin_time_utc = {},
        .end_time_utc = {},
    };
    statistics.emplace_back(statistics_2);  // XXX FIXME std::move
  }
  // mark price
  {
    auto statistics_2 = Statistics{
        .type = StatisticsType::PRE_SETTLEMENT_PRICE,
        .value = market_data_incremental_refresh.mark_price,
        .begin_time_utc = {},
        .end_time_utc = {},
    };
    statistics.emplace_back(statistics_2);  // XXX FIXME std::move
  }
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : market_data_incremental_refresh.no_md_entries) {
    if (exchange_time_utc < item.md_entry_date) {
      exchange_time_utc = item.md_entry_date;
    }
    switch (item.md_entry_type) {
      using enum roq::fix::MDEntryType;
      case BID:
        validate(item);
        emplace_back(mbp.bids, item);
        break;
      case OFFER:
        validate(item);
        emplace_back(mbp.asks, item);
        break;
      case TRADE:
        emplace_back(trades, item);
        break;
      case INDEX_VALUE: {
        auto statistics_2 = Statistics{
            .type = StatisticsType::INDEX_VALUE,
            .value = item.md_entry_px,
            .begin_time_utc = {},
            .end_time_utc = {},
        };
        statistics.emplace_back(statistics_2);  // XXX FIXME std::move
        break;
      }
      case SETTLEMENT_PRICE: {
        auto statistics_2 = Statistics{
            .type = StatisticsType::SETTLEMENT_PRICE,
            .value = item.md_entry_px,
            .begin_time_utc = {},
            .end_time_utc = {},
        };
        statistics.emplace_back(statistics_2);  // XXX FIXME std::move
        break;
      }
      default:
        log::warn("unsupported: {}"sv, item);
        break;
    }
  }
  if (!(std::empty(mbp.bids) && std::empty(mbp.asks)) && publish_market_by_price_) {
    if (latch_.find(symbol) == std::end(latch_)) {
      auto market_by_price_update = MarketByPriceUpdate{
          .stream_id = stream_id_,
          .exchange = exchange_,
          .symbol = symbol,
          .bids = mbp.bids,
          .asks = mbp.asks,
          .update_type = UpdateType::INCREMENTAL,
          .exchange_time_utc = exchange_time_utc,
          .exchange_sequence = {},
          .sending_time_utc = header.sending_time,
          .price_precision = {},
          .quantity_precision = {},
          .checksum = {},
      };
      auto is_last = std::empty(statistics) && std::empty(trades);
      try {
        create_trace_and_dispatch(handler_, trace_info, market_by_price_update, is_last);
      } catch (BadState &) {
        resubscribe(symbol);
      }
    }
  }
  if (!std::empty(trades) && publish_trade_summary_) {
    auto trade_summary = TradeSummary{
        .stream_id = stream_id_,
        .exchange = exchange_,
        .symbol = symbol,
        .trades = trades,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
        .sending_time_utc = header.sending_time,
    };
    auto is_last = std::empty(statistics);
    create_trace_and_dispatch(handler_, trace_info, trade_summary, is_last);
  }
  if (!std::empty(statistics)) {
    auto statistics_update = StatisticsUpdate{
        .stream_id = stream_id_,
        .exchange = exchange_,
        .symbol = market_data_incremental_refresh.symbol,
        .statistics = statistics,
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
        .sending_time_utc = header.sending_time,
    };
    create_trace_and_dispatch(handler_, trace_info, statistics_update, true);
  }
}

void MarketData::operator()(
    Trace<fix::MarketDataRequestReject> const &event, roq::fix::Header const &header) {  // NOLINT(readability-make-member-function-const)
  auto &[trace_info, market_data_request_reject] = event;
  log::warn<1>("event={{header={}, market_data_request_reject={}}}"sv, header, market_data_request_reject);
  if (shared_.settings.fix.terminate_on_market_data_request_reject) {
    log::fatal("Unexpected"sv);
  }
}

void MarketData::operator()(Trace<fix::MarketDataSnapshotFullRefresh> const &event, roq::fix::Header const &header) {
  auto &[trace_info, market_data_snapshot_full_refresh] = event;
  log::info<3>("event={{header={}, market_data_snapshot_full_refresh={}}}"sv, header, market_data_snapshot_full_refresh);
  (*connection_manager_).touch(trace_info.source_receive_time);
  auto symbol = market_data_snapshot_full_refresh.symbol;
  auto iter = latch_.find(symbol);
  if (iter != std::end(latch_)) [[unlikely]] {
    log::info<1>(R"(Unlatch symbol="{}")"sv, symbol);
    latch_.erase(iter);
  }
  auto &mbp = shared_.get_mbp();
  auto &statistics = shared_.get_statistics();
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : market_data_snapshot_full_refresh.no_md_entries) {
    if (exchange_time_utc < item.md_entry_date) {
      exchange_time_utc = item.md_entry_date;
    }
    switch (item.md_entry_type) {
      using enum roq::fix::MDEntryType;
      case BID:
        validate(item);
        emplace_back(mbp.bids, item);
        break;
      case OFFER:
        validate(item);
        emplace_back(mbp.asks, item);
        break;
      case TRADE:
        break;  // drop
      case INDEX_VALUE: {
        auto statistics_2 = Statistics{
            .type = StatisticsType::INDEX_VALUE,
            .value = item.md_entry_px,
            .begin_time_utc = {},
            .end_time_utc = {},
        };
        statistics.emplace_back(statistics_2);  // XXX FIXME std::move
        break;
      }
      case SETTLEMENT_PRICE: {
        auto statistics_2 = Statistics{
            .type = StatisticsType::SETTLEMENT_PRICE,
            .value = item.md_entry_px,
            .begin_time_utc = {},
            .end_time_utc = {},
        };
        statistics.emplace_back(statistics_2);  // XXX FIXME std::move
        break;
      }
      default:
        log::warn("unsupported: {}"sv, item);
        break;
    }
  }
  if (!(std::empty(mbp.bids) && std::empty(mbp.asks)) && publish_market_by_price_) {
    auto is_last = std::empty(statistics);
    auto market_by_price_update = MarketByPriceUpdate{
        .stream_id = stream_id_,
        .exchange = exchange_,
        .symbol = symbol,
        .bids = mbp.bids,
        .asks = mbp.asks,
        .update_type = UpdateType::SNAPSHOT,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
        .sending_time_utc = header.sending_time,
        .price_precision = {},
        .quantity_precision = {},
        .checksum = {},
    };
    try {
      create_trace_and_dispatch(handler_, trace_info, market_by_price_update, is_last);
    } catch (BadState &) {
      log::warn("market_by_price_update={}"sv, market_by_price_update);
      auto &bids = market_by_price_update.bids;
      auto &asks = market_by_price_update.asks;
      log::warn("best bid/ask={}/{}"sv, std::empty(bids) ? NaN : bids[0].price, std::empty(asks) ? NaN : asks[0].price);
      log::fatal(R"(*** BAD SNAPSHOT *** (symbol="{}"))"sv, symbol);
    }
  }
  if (!std::empty(statistics)) {
    auto statistics_update = StatisticsUpdate{
        .stream_id = stream_id_,
        .exchange = exchange_,
        .symbol = symbol,
        .statistics = statistics,
        .update_type = UpdateType::SNAPSHOT,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
        .sending_time_utc = header.sending_time,
    };
    create_trace_and_dispatch(handler_, trace_info, statistics_update, true);
  }
}

// utilities

template <typename T>
void MarketData::send(T const &event) {
  auto now = clock::get_realtime();
  send(event, now);
}

template <typename T>
void MarketData::send(T const &event, std::chrono::nanoseconds sending_time) {
  auto helper = [&](auto &message) { log::info("{}"sv, utils::debug::fix::Message{message}); };
  auto header = roq::fix::Header{
      .version = FIX_VERSION,
      .msg_type = T::MSG_TYPE,
      .sender_comp_id = SENDER_COMP_ID,
      .target_comp_id = TARGET_COMP_ID,
      .msg_seq_num = ++outbound_.msg_seq_num,  // note!
      .sending_time = sending_time,
  };
  if ((*connection_manager_).send([&](auto &buffer) {
        auto message = event.encode(header, buffer);
        if (fix_debug_) [[unlikely]] {
          helper(message);
        }
        return std::size(message);
      })) {
  } else {
    log::warn("HERE"sv);
  }
}

void MarketData::check(roq::fix::Header const &header) {
  auto current = header.msg_seq_num;
  auto expected = inbound_.msg_seq_num + 1;
  if (current != expected) [[unlikely]] {
    if (expected < current) {
      log::warn(
          "*** SEQUENCE GAP *** "
          "current={} previous={} distance={}"sv,
          current,
          inbound_.msg_seq_num,
          current - inbound_.msg_seq_num);
    } else {
      log::warn(
          "*** SEQUENCE REPLAY *** "
          "current={} previous={} distance={}"sv,
          current,
          inbound_.msg_seq_num,
          inbound_.msg_seq_num - current);
    }
  }
  inbound_.msg_seq_num = current;
}

}  // namespace deribit
}  // namespace roq
