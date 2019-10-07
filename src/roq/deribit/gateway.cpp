/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <gflags/gflags.h>

#include <limits>

#include "roq/logging.h"
#include "roq/format.h"
#include "roq/stream.h"

#include "roq/core/clock.h"

#include "roq/core/fix/utils.h"

#include "roq/deribit/random.h"

#include "roq/deribit/fix/heartbeat.h"
#include "roq/deribit/fix/logon.h"
#include "roq/deribit/fix/market_data_request.h"
#include "roq/deribit/fix/new_order_single.h"
#include "roq/deribit/fix/order_cancel_replace_request.h"
#include "roq/deribit/fix/order_cancel_request.h"
#include "roq/deribit/fix/order_mass_status_request.h"
#include "roq/deribit/fix/request_for_positions.h"
#include "roq/deribit/fix/security_list_request.h"
#include "roq/deribit/fix/user_request.h"

DEFINE_string(
    fix_uri,
    "tcp://test.deribit.com:9881",
    "FIX end-point (URI)");

DEFINE_uint64(ping_freq_secs, 5, "ping frequency (seconds)");
DEFINE_string(exchange, "deribit", "exchange identifier (string)");
DEFINE_bool(cancel_on_disconnect, true, "cancel orders on disconnect? (bool)");

DEFINE_int32(network_affinity, -1, "network (epoll) affinity");

DEFINE_uint32(max_depth, 65536, "maximum depth");

DEFINE_uint32(encode_buffer_size, 1048576, "encode buffer size");
DEFINE_uint32(decode_buffer_size, 1048576, "decode buffer size");

DEFINE_uint64(reconnect_secs, 3, "time before reconnect (seconds)");

// following options are work-arounds for weird behavior:

// - batch subscription doesn't seem to work (as of 2019-10-06)
DEFINE_bool(batch_subscribe, false, "batch subscribe symbols? (bool)");

// external
DECLARE_string(name);

namespace roq {
namespace deribit {

namespace {
constexpr auto TIMER_FREQUENCY = std::chrono::milliseconds{100};
constexpr auto LOGOUT_MESSAGE = "i'm done";
constexpr auto RESEND_MESSAGE = "resend_not_supported";
}  // namespace

// utilities

namespace {
static std::string create_latency_labels(
    const std::string_view& connection) {
  return fmt::format(
      "source=\"{}\", connection=\"{}\"",
      FLAGS_name,
      connection);
}
template <typename T, typename U>
static inline void mbp_update(
    auto& data,
    size_t& offset,
    const T& item,
    const U& action) {
  new (&data[offset++]) MBPUpdate {
    .price = item.md_entry_px,
    .quantity = item.md_entry_size,
    .action = action,
  };
  if (offset >= data.size())
    throw std::runtime_error("Not enough space");
}
}  // namespace

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const Config& config)
    : _dispatcher(dispatcher),
      _dns_base(_base, true),
      _timer(_base, EV_PERSIST, [this]() { on_timer(); }),
      _encode_buffer(FLAGS_encode_buffer_size),
      _access_key(config.get_access_key()),
      _access_secret(config.get_access_secret()),
      _symbols_regex(config.symbols),
      _bid(FLAGS_max_depth),
      _ask(FLAGS_max_depth),
      _account(config.get_account()),
      _fix_latency("roq_latency", create_latency_labels("fix")) {
}

void Gateway::operator()(const StartEvent&) {
  LOG(INFO) << "Starting the gateway event loop...";
  _thread = std::thread([this]() { run(); });
}

void Gateway::operator()(const StopEvent&) {
  LOG(INFO) << "Stopping the gateway event loop...";
  _stop.store(true, std::memory_order_release);
  if (_thread.joinable())
    _thread.join();
  LOG(INFO) << "The gateway event loop has stopped";
}

void Gateway::operator()(const TimerEvent&) {
}

void Gateway::operator()(const ConnectionStatusEvent&) {
}

void Gateway::operator()(const CreateOrderEvent&) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::operator()(const ModifyOrderEvent&) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::operator()(const CancelOrderEvent&) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::write(Metrics& metrics) const {
  _fix_latency.write(metrics);
}

void Gateway::run() {
  LOG(INFO) << "Gateway event loop has started";
  try {
    initialize_thread();
    _timer.add(TIMER_FREQUENCY);
    create_fix();
    _base.loop(EVLOOP_NO_EXIT_ON_EMPTY);
  } catch (std::exception& e) {
    LOG(FATAL) << "Unhandled exception, what=\"" << e.what() << "\"";
  } catch (...) {
    LOG(FATAL) << "Unhandled exception";
  }
  LOG(INFO) << "Gateway event loop has finished";
}

void Gateway::initialize_thread() {
  if (FLAGS_network_affinity >= 0) {
    LOG(INFO) << "Thread affinity " << FLAGS_network_affinity;
    set_thread_affinity(FLAGS_network_affinity);
  }
}

void Gateway::on_timer() {
  if (_stop.load(std::memory_order_acquire)) {
    _base.loopbreak();
    return;
  }
  auto now = core::get_time();
  auto ping = _next_update <= now;
  if (ping)
    _next_update = now + std::chrono::seconds{FLAGS_ping_freq_secs};
  switch (_gateway_status) {
    case GatewayStatus::DISCONNECTED:
      if (static_cast<bool>(_fix)) {
        _fix.reset();
        _fix_reconnect_time = now +
          std::chrono::seconds{ FLAGS_reconnect_secs };
      } else {
        if (_fix_reconnect_time < now) {
          assert(_fix_reconnect_time.count() > 0);
          _fix_reconnect_time = {};
          create_fix();
        }
      }
      break;
    case GatewayStatus::DOWNLOADING:
    case GatewayStatus::READY: {
      if (ping) {
        VLOG(4) << "Sending FIX TestRequest";
        auto test_req_id = fmt::format(  // FIXME(thraneh): use charconv
            "{}",
            core::get_system_clock().count());
        fix::TestRequest test_request {
          .test_req_id = test_req_id,
        };
        send(test_request);
      }
      break;
    }
    default:
      break;
  }
}

// FIX:

void Gateway::create_fix() {
  assert(_gateway_status == GatewayStatus::DISCONNECTED);
  assert(static_cast<bool>(_fix) == false);
  _fix = std::make_unique<FIX>(
      *this,
      _ssl_context,
      _base,
      _dns_base,
      core::URI(FLAGS_fix_uri),
      FLAGS_decode_buffer_size);
  _fix->start();
  update(GatewayStatus::CONNECTING);
}

void Gateway::on_fix_connected() {
  assert(_gateway_status == GatewayStatus::CONNECTING);
  LOG(INFO) << fmt::format(
      "[FIX] request logon (username=\"{}\")...", _access_key);
  auto sending_time = core::get_realtime_clock();
  auto raw_data = Random::create_raw_data(sending_time);
  auto password = Random::create_password(raw_data, _access_secret);
  fix::Logon logon = {
    .heart_bt_int = static_cast<uint16_t>(FLAGS_ping_freq_secs),
    .raw_data = raw_data,
    .username = _access_key,
    .password = password,
    .deribit_cancel_on_disconnect = FLAGS_cancel_on_disconnect,
  };
  send(logon, sending_time);
  update(GatewayStatus::LOGIN_SENT);
}

void Gateway::on_fix_disconnected() {
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  update(GatewayStatus::DISCONNECTED);
  _download = Download::NONE;
  reset();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::ExecutionReport& execution_report) {
  // FIXME(thraneh): something missing, we can't assert on _gateway_status
  VLOG(1) << fmt::format(
      "header={}, execution_report={}",
      header,
      execution_report);
  switch (execution_report.exec_type) {
    case core::fix::ExecType::ORDER_STATUS:
      assert(_gateway_status == GatewayStatus::READY);
      // TODO(thraneh): forward to gateway
      /*
      // DEBUG
      switch (execution_report.ord_status){
        case core::fix::OrdStatus::NEW:
          if (execution_report.order_qty > 1.0) {
            fix::OrderCancelReplaceRequest order_cancel_replace_request = {
              .cl_ord_id = execution_report.orig_cl_ord_id,  // TODO(thraneh): check order
              .orig_cl_ord_id = execution_report.cl_ord_id,
              .side = core::fix::Side::BUY,
              .order_qty = 1.0,
              .ord_type = core::fix::OrdType::LIMIT,
              .price = 1.0,
              .symbol = SYMBOL,
              .transact_time = execution_report.transact_time,
            };
            send(order_cancel_replace_request);
          } else {
            fix::OrderCancelRequest order_cancel_request = {
              .cl_ord_id = execution_report.orig_cl_ord_id,  // TODO(thraneh): check order
              .orig_cl_ord_id = execution_report.cl_ord_id,
            };
            send(order_cancel_request);
          }
          break;
        default:
          break;
      }
      */
      break;
    default:
      break;
  }

  // download?
  switch (execution_report.mass_status_req_type) {
    case core::fix::MassStatusReqType::ORDERS:
      assert(_gateway_status == GatewayStatus::DOWNLOADING);
      _download_execution_reports = execution_report.tot_num_reports;
      if (_download_execution_reports == 0) {
        check_download();
      } else {
        // wait for more execution reports...
      }
      break;
    default:
      if (_download_execution_reports &&
          0 == --_download_execution_reports)
        check_download();
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Heartbeat& heartbeat) {
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  VLOG(3) << fmt::format(
      "header={}, heartbeat={}",
      header,
      heartbeat);
  if (heartbeat.test_req_id.empty() == false) {
    auto send_time = core::charconv::from_string<uint64_t>(
        heartbeat.test_req_id);
    auto latency =
      std::chrono::duration_cast<std::chrono::microseconds>(
          now - decltype(now){send_time}) / 2;  // 1-way
    VLOG(1) << fmt::format("[FIX] latency={}", latency);
    _fix_latency.update(latency.count());
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Logon& logon) {
  assert(_gateway_status == GatewayStatus::LOGIN_SENT);
  VLOG(1) << fmt::format(
      "header={}, logon={}",
      header,
      logon);
  LOG(INFO) << "[FIX] logon COMPLETED";
  begin_download();
  /*
  // DEBUG
  fix::NewOrderSingle new_order_single = {
    .cl_ord_id = "roq-ord-006",
    .side = core::fix::Side::BUY,
    .order_qty = 2.0,
    .price = 0.5,
    .symbol = SYMBOL,
    .ord_type = core::fix::OrdType::LIMIT,
    .time_in_force = core::fix::TimeInForce::GTC,
    .deribit_label = "roq;123;345",
  };
  send(new_order_single);
  */
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Logout& logout) {
  assert(_gateway_status == GatewayStatus::READY);
  VLOG(1) << fmt::format(
      "header={}, logout={}",
      header,
      logout);
  LOG(WARNING) << fmt::format(
      "[FIX] logout (text=\"{}\")",
      logout.text);
  update(GatewayStatus::LOGGED_OUT);
  // note! mandated, must send a logout response
  fix::Logout response = {
    .text = LOGOUT_MESSAGE,
  };
  send(response);
  reset();
  LOG(FATAL) << "Unexpected -- now what?";  // FIXME(thraneh): ...
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
  assert(_gateway_status == GatewayStatus::READY);
  VLOG(3) << fmt::format(
      "header={}, market_data_incremental_refresh={}",
      header,
      market_data_incremental_refresh);
  size_t bid_length = 0, ask_length = 0;
  std::chrono::nanoseconds exchange_time_utc = {};
  auto& md_inc_grp = market_data_incremental_refresh.md_inc_grp;
  for (size_t i = 0; i < md_inc_grp.length; ++i) {
    auto& item = md_inc_grp.items[i];
    if (item.md_entry_date > exchange_time_utc)
      exchange_time_utc = item.md_entry_date;
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        mbp_update(_bid, bid_length, item, core::fix::map(item.md_update_action));
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        mbp_update(_ask, ask_length, item, core::fix::map(item.md_update_action));
        break;
      }
      case core::fix::MDEntryType::TRADE: {
        TradeSummary trade_summary = {
          .exchange = FLAGS_exchange,
          .symbol = market_data_incremental_refresh.symbol,
          .price = item.md_entry_px,
          .volume = item.md_entry_size,
          .turnover = std::numeric_limits<double>::quiet_NaN(),
          .side = core::fix::map(item.side),
          .exchange_time_utc = exchange_time_utc,
        };
        enqueue(trade_summary, true);  // FIXME(thraneh): *not* always last
        break;
      }
      case core::fix::MDEntryType::INDEX_VALUE:
      case core::fix::MDEntryType::SETTLEMENT_PRICE:
        // FIXME(thraneh): how to propagate these???
        VLOG(1) << fmt::format("[FIX] unsupported: {}", item);
        break;
      default:
        LOG(WARNING) << fmt::format("[FIX] unsupported: {}", item);
        break;
    }
  }
  if (bid_length > 0 || ask_length > 0) {
    MarketByPrice market_by_price = {
      .exchange = FLAGS_exchange,
      .symbol = market_data_incremental_refresh.symbol,
      .bid_length = bid_length,
      .bid = _bid.data(),
      .ask_length = ask_length,
      .ask = _ask.data(),
      .snapshot = false,  // incremental
      .exchange_time_utc = exchange_time_utc,
    };
    enqueue(market_by_price, true);
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataRequestReject& market_data_request_reject) {
  assert(_gateway_status == GatewayStatus::READY);
  VLOG(1) << fmt::format(
      "header={}, market_data_request_reject={}",
      header,
      market_data_request_reject);
  LOG(WARNING) << fmt::format(
      "[FIX] market data request reject (reason={}, text=\"{}\")",
      market_data_request_reject.md_req_rej_reason,
      market_data_request_reject.text);
  LOG(FATAL) << "Unexpected -- now what?";  // FIXME(thraneh): ...
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
  assert(_gateway_status == GatewayStatus::READY);
  VLOG(3) << fmt::format(
      "header={}, market_data_snapshot_full_refresh={}",
      header,
      market_data_snapshot_full_refresh);
  LOG(INFO) << fmt::format(
      "Market data snapshot symbol=\"{}\"",
      market_data_snapshot_full_refresh.symbol);
  size_t bid_length = 0, ask_length = 0;
  auto& md_full_grp = market_data_snapshot_full_refresh.md_full_grp;
  for (size_t i = 0; i < md_full_grp.length; ++i) {
    auto& item = md_full_grp.items[i];
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        mbp_update(_bid, bid_length, item, UpdateAction::NEW);
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        mbp_update(_ask, ask_length, item, UpdateAction::NEW);
        break;
      }
      default:
        break;
    }
  }
  if (bid_length == 0 && ask_length == 0) return;  // TODO(thraneh): check roq-server support
  MarketByPrice market_by_price = {
    .exchange = FLAGS_exchange,
    .symbol = market_data_snapshot_full_refresh.symbol,
    .bid_length = bid_length,
    .bid = _bid.data(),
    .ask_length = ask_length,
    .ask = _ask.data(),
    .snapshot = true,  // reset
    .exchange_time_utc = {},
  };
  enqueue(market_by_price, true);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::OrderCancelReject& order_cancel_reject) {
  assert(_gateway_status == GatewayStatus::READY);
  VLOG(1) << fmt::format(
      "header={}, order_cancel_reject={}",
      header,
      order_cancel_reject);
  // TODO(thraneh): forward to gateway
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::PositionReport& position_report) {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  VLOG(1) << fmt::format(
      "header={}, position_report={}",
      header,
      position_report);
  // TODO(thraneh): forward to gateway
  check_download();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Reject& reject) {
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  VLOG(1) << fmt::format(
      "header={}, reject={}",
      header,
      reject);
  LOG(WARNING) << fmt::format(
      "[FIX] reject (msg_type=\"{}\", text=\"{}\")",
      reject.ref_msg_type,
      reject.text);
  LOG(FATAL) << "Unexpected -- now what?";  // FIXME(thraneh): ...
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::ResendRequest& resend_request) {
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  VLOG(1) << fmt::format(
      "header={}, resend_request={}",
      header,
      resend_request);
  fix::Reject reject = {
    .ref_seq_num = header.msg_seq_num,
    .ref_tag_id = 0,
    .ref_msg_type = header.msg_type_raw,
    .text = RESEND_MESSAGE,
  };
  send(reject);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::SecurityList& security_list) {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  VLOG(3) << fmt::format(
      "header={}, security_list={}",
      header,
      security_list);
  if (security_list.instruments.length) {
    assert(_symbols.empty());
    _symbols.reserve(security_list.instruments.length);  // note! alloc
    for (size_t i = 0; i < security_list.instruments.length; ++i) {
      auto& instrument = security_list.instruments.items[i];
      if (discard_symbol(instrument.symbol))
        continue;
      _symbols.emplace_back(instrument.symbol);
      ReferenceData reference_data = {
        .exchange = FLAGS_exchange,
        .symbol = instrument.symbol,
        .tick_size = instrument.min_price_increment,
        .limit_up = std::numeric_limits<double>::quiet_NaN(),
        .limit_down = std::numeric_limits<double>::quiet_NaN(),
        .multiplier = instrument.contract_multiplier,
      };
      enqueue(reference_data, false);
      // note! we receive no information about the trading status
      MarketStatus market_status = {
        .exchange = FLAGS_exchange,
        .symbol = instrument.symbol,
        .trading_status = TradingStatus::OPEN,  // TODO(thraneh): missing
      };
      enqueue(market_status, true);
    }
  }
  check_download();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::TestRequest& test_request) {
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  VLOG(1) << fmt::format(
      "header={}, test_request={}",
      header,
      test_request);
  fix::Heartbeat heartbeat = {
    .test_req_id = test_request.test_req_id,
  };
  send(heartbeat);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::UserResponse& user_response) {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  VLOG(1) << fmt::format(
      "header={}, user_response={}",
      header,
      user_response);
  // TODO(thraneh): forward to gateway
  check_download();
}

// UTILS:

inline bool Gateway::discard_symbol(const std::string_view& symbol) {
  for (auto& regex : _symbols_regex)
    if (std::regex_match(symbol.begin(), symbol.end(), regex)) {
      return false;
    }
  VLOG(4) << fmt::format(
      "Discard symbol=\"{}\" (reason: no regex match)", symbol);
  return true;
}

void Gateway::update(GatewayStatus gateway_status) {
  _gateway_status = gateway_status;
  MarketDataStatus market_data_status = {
    .status = _gateway_status,
  };
  enqueue(market_data_status, false);
  OrderManagerStatus order_manager_status = {
    .account = _account.c_str(),
    .status = _gateway_status,
  };
  enqueue(order_manager_status, true);
  LOG(INFO) << fmt::format("gateway_status={}", _gateway_status);
}

void Gateway::begin_download() {
  assert(_download == Download::NONE);
  assert(_gateway_status == GatewayStatus::LOGIN_SENT);
  update(GatewayStatus::DOWNLOADING);
  LOG(INFO) << "[FIX] download:";
  download_securities();
}

void Gateway::check_download() {
  assert(_download != Download::NONE);
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  switch (_download) {
    case Download::NONE:
      assert(false);
      break;
    case Download::SECURITIES:
      LOG(INFO) << "[FIX] download securities COMPLETED";
      download_positions();
      break;
    case Download::POSITIONS:
      LOG(INFO) << "[FIX] download positions COMPLETED";
      download_orders();
      break;
    case Download::ORDERS:
      LOG(INFO) << "[FIX] download orders COMPLETED";
      download_user();
      break;
    case Download::USER: {
      LOG(INFO) << "[FIX] download user COMPLETED";
      update(GatewayStatus::READY);
      LOG(INFO) << "[FIX] download COMPLETED";
      _download = Download::NONE;
      subscribe_market_data();
      break;
    };
  }
}

void Gateway::download_securities() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO) << "[FIX] download securities...";
  auto security_req_id = get_next_request_id();
  fix::SecurityListRequest security_list_request = {
    .security_req_id = security_req_id,
  };
  send(security_list_request);
  _download = Download::SECURITIES;
}

void Gateway::download_positions() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO) << "[FIX] download positions...";
  auto pos_req_id = get_next_request_id();
  fix::RequestForPositions request_for_positions = {
    .pos_req_id = pos_req_id,
    .pos_req_type = core::fix::PosReqType::POSITIONS,
  };
  send(request_for_positions);
  _download = Download::POSITIONS;
}

void Gateway::download_orders() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO) << "[FIX] download orders...";
  auto mass_status_req_id = get_next_request_id();
  fix::OrderMassStatusRequest order_mass_status_request = {
    .mass_status_req_id = mass_status_req_id,
    .mass_status_req_type = core::fix::MassStatusReqType::ORDERS,
  };
  send(order_mass_status_request);
  _download = Download::ORDERS;
}

void Gateway::download_user() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO) << "[FIX] download user...";
  auto user_request_id = get_next_request_id();
  fix::UserRequest user_request = {
    .user_request_id = user_request_id,
    .username = _access_key,
  };
  send(user_request);
  _download = Download::USER;
}

void Gateway::reset() {
  _download = Download::NONE;
  _symbols.clear();
}

void Gateway::subscribe_market_data() {
  assert(_gateway_status == GatewayStatus::READY);
  if (_symbols.empty()) {
    LOG(WARNING) << "Can't subscribe market data, reason: NO SYMBOLS";
    return;
  }
  LOG(INFO) << "Subscribe market data";
  auto md_req_id = get_next_request_id();
  if (FLAGS_batch_subscribe) {
    std::vector<std::string_view> symbols(_symbols.size());
    for (size_t i = 0; i < _symbols.size(); ++i)
      symbols[i] = _symbols[i];
    fix::MarketDataRequest market_data_request = {
      .md_req_id = md_req_id,
      .symbol = {},
      .symbols = symbols,
    };
    send(market_data_request);
  } else {
    for (auto& symbol : _symbols) {
      auto md_req_id = get_next_request_id();
      fix::MarketDataRequest market_data_request = {
        .md_req_id = md_req_id,
        .symbol = symbol,
        .symbols = {},
      };
      send(market_data_request);
    }
  }
}

std::string Gateway::get_next_request_id() {
  return fmt::format(  // FIXME(thraneh): use charconv
      "roq:{:09}", ++_request_id);
}

}  // namespace deribit
}  // namespace roq
