/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <gflags/gflags.h>

#include <limits>

#include "roq/logging.h"
#include "roq/format.h"

#include "roq/core/clock.h"

#include "roq/core/fix/utils.h"

#include "roq/core/metrics/profile.h"

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

DEFINE_string(fix_uri, "tcp://test.deribit.com:9881",
    "FIX end-point (URI)");

DEFINE_uint64(ping_freq_secs, 5,
    "ping frequency (seconds)");

DEFINE_string(exchange, "deribit",
    "exchange identifier (string)");

DEFINE_bool(cancel_on_disconnect, true,
    "cancel orders on disconnect? (bool)");

DEFINE_bool(silence_empty_messages, true,
    "silence empty messages? (bool)");

DEFINE_uint32(max_depth, 65536,
    "maximum depth for market by price");

DEFINE_uint32(max_trades, 256,
    "maximum trades for trade summary");

DEFINE_uint32(encode_buffer_size, 1048576,
    "encode buffer size");

DEFINE_uint32(decode_buffer_size, 1048576,
    "decode buffer size");

DEFINE_uint64(reconnect_secs, 10,
    "time before reconnect (seconds)");

// following options are work-arounds for weird behavior:

// - batch subscription doesn't seem to work (as of 2019-10-06)
DEFINE_bool(batch_subscribe, false,
    "batch subscribe symbols? (bool)");

// external
DECLARE_string(name);

namespace roq {
namespace deribit {

namespace {
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
static auto create_histogram(const std::string_view& function) {
  auto labels = fmt::format(
      "source=\"{}\", function=\"{}\"",
      FLAGS_name,
      function);
  return Gateway::histogram_t("roq_profile", labels.c_str());
}
template <typename T>
static inline void mbp_update(
    auto& data,
    size_t& offset,
    const T& item) {
  // validate
  switch (item.md_update_action) {
    case core::fix::MDUpdateAction::UNKNOWN:
      break;
    case core::fix::MDUpdateAction::NEW:
    case core::fix::MDUpdateAction::CHANGE:
      assert(std::fabs(item.md_entry_size) >= 1.0e-10);
      break;
    case core::fix::MDUpdateAction::DELETE:
      assert(std::fabs(item.md_entry_size) < 1.0e-10);
      break;
    case core::fix::MDUpdateAction::DELETE_THRU:
    case core::fix::MDUpdateAction::DELETE_FROM:
      throw std::runtime_error("MDUpdateAction not supported");
      break;
  }
  new (&data[offset++]) MBPUpdate {
    .price = item.md_entry_px,
    .quantity = item.md_entry_size,
  };
  if (offset >= data.size())
    throw std::runtime_error("Not enough space");
}
template <typename T>
static inline void trade_update(
    auto& data,
    size_t& offset,
    const T& item) {
  auto& trade = data[offset++];
  new (&trade) Trade {
    .side = core::fix::map(item.side),
    .price = item.md_entry_px,
    .quantity = item.md_entry_size,
    .trade_id = {},  // copy string (following statement)
  };
  item.deribit_trade_id.copy(trade.trade_id, sizeof(trade.trade_id));
  if (offset >= data.size())
    throw std::runtime_error("Not enough space");
}
static inline std::pair<uint32_t, uint32_t> parse_deribit_label(
    const std::string_view&) {
  return std::make_pair(uint32_t{0}, uint32_t{0});
}
}  // namespace

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const Config& config)
    : _dispatcher(dispatcher),
      _dns_base(_base, true),
      _encode_buffer(FLAGS_encode_buffer_size),
      _access_key(config.get_access_key()),
      _access_secret(config.get_access_secret()),
      _symbols_regex(config.symbols),
      _bid(FLAGS_max_depth),
      _ask(FLAGS_max_depth),
      _trade(FLAGS_max_trades),
      _account(config.get_account()),
      _fix_latency("roq_latency", create_latency_labels("fix")),
      _market_data_incremental_refresh(
          create_histogram("market_data_incremental_refresh")) {
}

void Gateway::operator()(const StartEvent&) {
  LOG(INFO)("Starting the gateway...");
  create_fix();
}

void Gateway::operator()(const StopEvent&) {
  LOG(INFO)("Stopping the gateway...");
  // FIXME(thraneh): send logout here...
}

void Gateway::operator()(const TimerEvent& event) {
  auto now = event.now;
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
        VLOG(4)("FIX sending test request");
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
  _base.loop(EVLOOP_NONBLOCK);
}

void Gateway::operator()(const ConnectionStatusEvent&) {
}

void Gateway::operator()(const CreateOrderEvent& event) {
  auto& create_order = event.create_order;
  // FIXME(thraneh): check ready-state
  // validate
  if (create_order.account.compare(_account) != 0)
    throw std::runtime_error("Invalid account");
  if (create_order.exchange.compare(FLAGS_exchange) != 0)
    throw std::runtime_error("Invalid exchange");
  if (create_order.position_effect != PositionEffect::UNDEFINED)
    throw std::runtime_error("Invalid position effect");
  if (create_order.order_template.empty() == false &&
      create_order.order_template.compare("default") != 0)
    throw std::runtime_error("Invalid order template");
  // generate id's
  std::string cl_ord_id = "roq-ord-006";  // FIXME(thraneh): implement
  std::string deribit_label = "roq;123;345";  // FIXME(thraneh): implement
  // create request
  fix::NewOrderSingle new_order_single = {
    .cl_ord_id = cl_ord_id,
    .side = core::fix::map(create_order.side),
    .order_qty = create_order.quantity,
    .price = create_order.price,
    .symbol = create_order.symbol,
    .ord_type = core::fix::map(create_order.order_type),
    .time_in_force = core::fix::map(create_order.time_in_force),
    .deribit_label = deribit_label,
  };
  send(new_order_single);
}

void Gateway::operator()(const ModifyOrderEvent& event) {
  auto& modify_order = event.modify_order;
  // FIXME(thraneh): check ready-state
  // validate
  if (modify_order.account.compare(_account) != 0)
    throw std::runtime_error("Invalid account");
  // cached id's
  // TODO(thraneh): use modify_order.order_id to lookup cached state
  std::string orig_cl_ord_id = "roq-ord-006";  // FIXME(thraneh): use cache (execution_report.cl_ord_id)
  // generate id's
  std::string cl_ord_id = "roq-ord-007";  // FIXME(thraneh): new? or use cached? (execution_report.orig_cl_ord_id)
  // missing: (cache?)
  auto side = Side::BUY;
  auto order_type = OrderType::LIMIT;
  std::string_view symbol = "BTC-XXX";
  std::chrono::nanoseconds transact_time = {};  // FIXME(thraneh): use cached? (execution_report.transact_time,)
  // create request
  fix::OrderCancelReplaceRequest order_cancel_replace_request = {
    .cl_ord_id = cl_ord_id,
    .orig_cl_ord_id = orig_cl_ord_id,
    .side = core::fix::map(side),
    .order_qty = modify_order.quantity,
    .ord_type = core::fix::map(order_type),
    .price = modify_order.price,
    .symbol = symbol,
    .transact_time = transact_time,
  };
  send(order_cancel_replace_request);
}

void Gateway::operator()(const CancelOrderEvent& event) {
  auto& cancel_order = event.cancel_order;
  // FIXME(thraneh): check ready-state
  // validate
  if (cancel_order.account.compare(_account) != 0)
    throw std::runtime_error("Invalid account");
  // cached id's
  // TODO(thraneh): use cancel_order.order_id to lookup cached state
  std::string orig_cl_ord_id = "roq-ord-006";  // FIXME(thraneh): use cache (execution_report.cl_ord_id)
  // generate id's
  std::string cl_ord_id = "roq-ord-007";  // FIXME(thraneh): new? or use cached? (execution_report.orig_cl_ord_id)
  // create request
  fix::OrderCancelRequest order_cancel_request = {
    .cl_ord_id = cl_ord_id,
    .orig_cl_ord_id = orig_cl_ord_id,
  };
  send(order_cancel_request);
}

void Gateway::write(Metrics& metrics) {
  metrics
    .write(_fix_latency)
    .write(_market_data_incremental_refresh);
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
  LOG(INFO)(
      "FIX sending logon request (username=\"{}\")...", _access_key);
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
  VLOG(1)(
      "FIX event(header={}, execution_report={})",
      header,
      execution_report);
  switch (execution_report.exec_type) {
    case core::fix::ExecType::PENDING_NEW:  // TODO(thraneh): does this exist?
      assert(_gateway_status == GatewayStatus::READY);
      // TODO(thraneh): CreateOrderAck
      break;
    /*
    case core::fix::ExecType::PENDING_REPLACE:  // TODO(thraneh): does this exist?
      assert(_gateway_status == GatewayStatus::READY);
      // TODO(thraneh): ModifyOrderAck
      break;
    */
    case core::fix::ExecType::PENDING_CANCEL:  // TODO(thraneh): does this exist?
      assert(_gateway_status == GatewayStatus::READY);
      // TODO(thraneh): CancelOrderAck
      break;
    case core::fix::ExecType::ORDER_STATUS:
      assert(_gateway_status == GatewayStatus::READY);
      // TODO(thraneh): OrderUpdate
      break;
    case core::fix::ExecType::TRADE:  // TODO(thraneh): does this exist?
      assert(_gateway_status == GatewayStatus::READY);
      // TODO(thraneh): TradeUpdate
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

  if (unlikely(execution_report.deribit_label.empty())) {
    LOG(WARNING)(
        "ExecutionReport being dropped. Reason: "
        "did not contain a deribit_label");
    return;
  }
  auto [order_id, order_local_id] =
    parse_deribit_label(execution_report.deribit_label);
  // FIXME(thraneh): update cache
  OrderUpdate order_update {
    .account = _account,
    .order_id = order_id,
    .exchange = FLAGS_exchange,
    .symbol = execution_report.symbol,
    .order_status = core::fix::map(execution_report.ord_status),
    .side = core::fix::map(execution_report.side),
    .price = execution_report.price,
    .remaining_quantity = execution_report.leaves_qty,
    .traded_quantity = execution_report.cum_qty,
    .position_effect = PositionEffect::UNDEFINED,
    .order_template = "",
    .insert_time_utc = {},
    .cancel_time_utc = {},
    .order_local_id = order_local_id,
    .order_external_id = execution_report.cl_ord_id,
  };

  // TODO(thraneh): lookup user_id
  auto user_id = uint8_t{0};  // FIXME(thraneh): how?

  enqueue(user_id, order_update, true);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Heartbeat& heartbeat) {
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  VLOG(3)(
      "FIX event(header={}, heartbeat={})",
      header,
      heartbeat);
  if (heartbeat.test_req_id.empty() == false) {
    auto send_time = core::charconv::from_string<uint64_t>(
        heartbeat.test_req_id);
    auto latency =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          now - decltype(now){send_time}) / 2;  // 1-way
    _fix_latency.update(latency.count());
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Logon& logon) {
  assert(_gateway_status == GatewayStatus::LOGIN_SENT);
  VLOG(1)(
      "FIX event(header={}, logon={})",
      header,
      logon);
  LOG(INFO)("FIX logon COMPLETED");
  begin_download();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Logout& logout) {
  assert(_gateway_status == GatewayStatus::READY);
  VLOG(1)(
      "FIX event(header={}, logout={})",
      header,
      logout);
  LOG(WARNING)(
      "FIX logout (text=\"{}\")",
      logout.text);
  update(GatewayStatus::LOGGED_OUT);
  // note! mandated, must send a logout response
  fix::Logout response = {
    .text = LOGOUT_MESSAGE,
  };
  send(response);
  LOG(INFO)("FIX closing connection");
  _fix->stop();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
  core::metrics::profile(_market_data_incremental_refresh, [&]() {
    assert(_gateway_status == GatewayStatus::READY);
    if (unlikely(FLAGS_silence_empty_messages &&
          market_data_incremental_refresh.md_inc_grp.length == 0))
      return;
    VLOG(3)(
        "FIX event(header={}, market_data_incremental_refresh={})",
        header,
        market_data_incremental_refresh);
    size_t bid_length = 0, ask_length = 0, trade_length = 0;
    std::chrono::nanoseconds exchange_time_utc = {};
    auto& md_inc_grp = market_data_incremental_refresh.md_inc_grp;
    for (size_t i = 0; i < md_inc_grp.length; ++i) {
      auto& item = md_inc_grp.items[i];
      if (item.md_entry_date > exchange_time_utc)
        exchange_time_utc = item.md_entry_date;
      switch (item.md_entry_type) {
        case core::fix::MDEntryType::BID: {
          mbp_update(_bid, bid_length, item);
          break;
        }
        case core::fix::MDEntryType::OFFER: {
          mbp_update(_ask, ask_length, item);
          break;
        }
        case core::fix::MDEntryType::TRADE: {
          trade_update(_trade, trade_length, item);
          break;
        }
        case core::fix::MDEntryType::INDEX_VALUE:
        case core::fix::MDEntryType::SETTLEMENT_PRICE:
          // FIXME(thraneh): how to propagate these???
          VLOG(1)("FIX unsupported: {}", item);
          break;
        default:
          LOG(WARNING)("FIX unsupported: {}", item);
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
    if (trade_length > 0) {
      TradeSummary trade_summary = {
        .exchange = FLAGS_exchange,
        .symbol = market_data_incremental_refresh.symbol,
        .trade_length = trade_length,
        .trade = _trade.data(),
        .exchange_time_utc = exchange_time_utc,
      };
      enqueue(trade_summary, true);  // FIXME(thraneh): *not* always last
    }
  });
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataRequestReject& market_data_request_reject) {
  assert(_gateway_status == GatewayStatus::READY);
  VLOG(1)(
      "FIX event(header={}, market_data_request_reject={})",
      header,
      market_data_request_reject);
  LOG(WARNING)(
      "FIX market data request reject (reason={}, text=\"{}\")",
      market_data_request_reject.md_req_rej_reason,
      market_data_request_reject.text);
  LOG(FATAL)("Unexpected -- now what?");  // FIXME(thraneh): ...
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
  assert(_gateway_status == GatewayStatus::READY);
  VLOG(3)(
      "FIX event(header={}, market_data_snapshot_full_refresh={})",
      header,
      market_data_snapshot_full_refresh);
  LOG(INFO)(
      "Market data snapshot symbol=\"{}\"",
      market_data_snapshot_full_refresh.symbol);
  size_t bid_length = 0, ask_length = 0;
  auto& md_full_grp = market_data_snapshot_full_refresh.md_full_grp;
  for (size_t i = 0; i < md_full_grp.length; ++i) {
    auto& item = md_full_grp.items[i];
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        mbp_update(_bid, bid_length, item);
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        mbp_update(_ask, ask_length, item);
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
  VLOG(1)(
      "FIX event(header={}, order_cancel_reject={})",
      header,
      order_cancel_reject);
  // TODO(thraneh): forward to gateway
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::PositionReport& position_report) {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  VLOG(1)(
      "FIX event(header={}, position_report={})",
      header,
      position_report);
  // TODO(thraneh): forward to gateway
  check_download();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Reject& reject) {
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  VLOG(1)(
      "FIX event(header={}, reject={})",
      header,
      reject);
  LOG(WARNING)(
      "FIX reject (msg_type=\"{}\", text=\"{}\")",
      reject.ref_msg_type,
      reject.text);
  LOG(FATAL)("Unexpected -- now what?");  // FIXME(thraneh): ...
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::ResendRequest& resend_request) {
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  VLOG(1)(
      "FIX event(header={}, resend_request={})",
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
  VLOG(3)(
      "FIX event(header={}, security_list={})",
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
        .min_trade_vol = instrument.min_trade_vol,
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
  VLOG(1)(
      "FIX event(header={}, test_request={})",
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
  VLOG(1)(
      "FIX event(header={}, user_response={})",
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
  VLOG(4)(
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
    .account = _account,
    .status = _gateway_status,
  };
  enqueue(order_manager_status, true);
  LOG(INFO)("gateway_status={}", _gateway_status);
}

void Gateway::begin_download() {
  assert(_download == Download::NONE);
  assert(_gateway_status == GatewayStatus::LOGIN_SENT);
  update(GatewayStatus::DOWNLOADING);
  LOG(INFO)("Download:");
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
      LOG(INFO)("Download securities COMPLETED");
      download_positions();
      break;
    case Download::POSITIONS:
      LOG(INFO)("Download positions COMPLETED");
      download_orders();
      break;
    case Download::ORDERS:
      LOG(INFO)("Download orders COMPLETED");
      download_user();
      break;
    case Download::USER: {
      LOG(INFO)("Download user COMPLETED");
      update(GatewayStatus::READY);
      LOG(INFO)("Download COMPLETED");
      _download = Download::NONE;
      subscribe_market_data();
      break;
    };
  }
}

void Gateway::download_securities() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("[FIX] download securities...");
  auto security_req_id = get_next_request_id();
  fix::SecurityListRequest security_list_request = {
    .security_req_id = security_req_id,
  };
  send(security_list_request);
  _download = Download::SECURITIES;
}

void Gateway::download_positions() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("[FIX] download positions...");
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
  LOG(INFO)("[FIX] download orders...");
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
  LOG(INFO)("[FIX] download user...");
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
    LOG(WARNING)("Can't subscribe market data, reason: NO SYMBOLS");
    return;
  }
  LOG(INFO)("Subscribe market data");
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
