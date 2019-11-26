/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <gflags/gflags.h>

#include <limits>
#include <utility>

#include "roq/logging.h"
#include "roq/format.h"

#include "roq/core/charconv.h"
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
#include "roq/deribit/fix/utils.h"

DEFINE_string(ws_uri,
    "wss://test.deribit.com/ws/api/v2",
    "WebSocket end-point (URI)");

DEFINE_string(fix_uri,
    "tcp://test.deribit.com:9881",
    "FIX end-point (URI)");

DEFINE_uint64(ping_freq_secs,
    uint64_t{5},
    "ping frequency (seconds)");

DEFINE_string(exchange,
    "deribit",
    "exchange identifier (string)");

DEFINE_bool(cancel_on_disconnect,
    true,
    "cancel orders on disconnect? (bool)");

DEFINE_bool(silence_empty_messages,
    true,
    "silence empty messages? (bool)");

DEFINE_uint32(max_trades,
    uint32_t{256},
    "maximum trades for trade summary");

DEFINE_uint32(encode_buffer_size,
    uint32_t{1048576},
    "encode buffer size");

DEFINE_uint32(decode_buffer_size,
    uint32_t{1048576},
    "decode buffer size");

DEFINE_uint64(reconnect_secs,
    {10},
    "time before reconnect (seconds)");

// following options are work-arounds for weird behavior:

// - batch subscription doesn't seem to work (as of 2019-10-06)
DEFINE_bool(batch_subscribe,
    true,
    "batch subscribe symbols? (bool)");

DEFINE_uint32(max_batch_size,
    56,
    "max batch size");

// external
DECLARE_string(name);
DECLARE_uint32(max_depth);

#define FIX_PREFIX "[FIX] "

namespace roq {
namespace deribit {

// utilities

namespace {
constexpr std::string_view LOGOUT_RESPONSE("LOGOUT_RESPONSE");
constexpr std::string_view RESEND_NOT_SUPPORTED("RESEND_NOT_SUPPORTED");
constexpr auto TOLERANCE = double{1.0e-10};
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
      assert(std::fabs(item.md_entry_size) >= TOLERANCE);
      break;
    case core::fix::MDUpdateAction::DELETE:
      assert(std::fabs(item.md_entry_size) < TOLERANCE);
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
  LOG_IF(WARNING, FLAGS_cancel_on_disconnect == false)(
      "Orders will *NOT* be cancelled on disconnect");
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
        VLOG(4)(FIX_PREFIX "sending test request");
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
  auto& message_info = event.message_info;
  auto& create_order = event.create_order;
  constexpr auto type = RequestType::CREATE_ORDER;
  constexpr auto origin = Origin::GATEWAY;
  auto error = Error::NONE;
  decltype(OrderAck::gateway_order_id) gateway_order_id = {};
  std::string request_id;
  if (unlikely(_gateway_status != GatewayStatus::READY)) {
    error = Error::GATEWAY_NOT_READY;
  } else if (unlikely(
        create_order.account.compare(_account) != 0)) {
    error = Error::INVALID_ACCOUNT;
  } else if (unlikely(
        create_order.exchange.compare(FLAGS_exchange) != 0)) {
    error = Error::INVALID_EXCHANGE;
  } else if (unlikely(
        create_order.position_effect != PositionEffect::UNDEFINED)) {
    error = Error::INVALID_POSITION_EFFECT;
  } else if (unlikely(
        create_order.order_template.empty() == false &&
        create_order.order_template.compare("default") != 0)) {
    error = Error::INVALID_ORDER_TEMPLATE;
  } else {
    // TODO(thraneh): check against max_order_id before continuing
    gateway_order_id = create_order_id();
    OrderMapping order_mapping(
        message_info,
        create_order,
        gateway_order_id);
    auto key = order_mapping.key();
    if (unlikely(_order_mapping.find(key) != _order_mapping.end())) {
      error = Error::INVALID_ORDER_ID;
    } else {
      request_id = create_request_id();
      auto iter = _order_mapping.emplace(
          key,
          std::move(order_mapping)).first;
      auto& order_mapping = (*iter).second;
      _order_lookup.emplace(request_id, key);
      fix::NewOrderSingle request {
        .cl_ord_id = request_id,
        .side = core::fix::map(create_order.side),
        .order_qty = create_order.quantity,
        .price = create_order.price,
        .symbol = create_order.symbol,
        .ord_type = core::fix::map(create_order.order_type),
        .time_in_force = core::fix::map(create_order.time_in_force),
        .deribit_label = (*iter).second.user_custom(),
      };
      try {
        send(request);
        order_mapping.update_request(request_id, type);
      } catch (std::exception& e) {
        LOG(WARNING)("Exception: what=\"{}\"", e.what());
        error = Error::NETWORK_ERROR;
      }
    }
  }
  OrderAck order_ack {
    .account = create_order.account,
    .order_id = create_order.order_id,
    .type = type,
    .origin = origin,
    .status = (error == Error::NONE)
      ? RequestStatus::FORWARDED
      : RequestStatus::REJECTED,
    .error = error,
    .text = std::string_view(),
    .gateway_order_id = gateway_order_id,
    .external_order_id = std::string_view(),
    .request_id = request_id,
  };
  _dispatcher.enqueue(
      message_info.source,
      order_ack,
      message_info.source_receive_time,
      message_info.origin_create_time,
      true);
}

void Gateway::operator()(const ModifyOrderEvent& event) {
  auto& message_info = event.message_info;
  auto& modify_order = event.modify_order;
  constexpr auto type = RequestType::MODIFY_ORDER;
  constexpr auto origin = Origin::GATEWAY;
  auto error = Error::NONE;
  decltype(OrderAck::gateway_order_id) gateway_order_id = {};
  decltype(OrderAck::external_order_id) external_order_id = {};
  std::string request_id;
  if (unlikely(_gateway_status != GatewayStatus::READY)) {
    error = Error::GATEWAY_NOT_READY;
  } else if (unlikely(
        modify_order.account.compare(_account) != 0)) {
    error = Error::INVALID_ACCOUNT;
  } else {
    auto key = OrderMapping::key(
        message_info.source,
        modify_order.order_id);
    auto iter = _order_mapping.find(key);
    if (unlikely(iter == _order_mapping.end())) {
      error = Error::UNKNOWN_ORDER_ID;
    } else {
      auto& order_mapping = (*iter).second;
      gateway_order_id = order_mapping.gateway_order_id();
      external_order_id = order_mapping.exchange_order_id();
      if (unlikely(order_mapping.ready() == false)) {
        error = Error::UNKNOWN_EXCHANGE_ORDER_ID;
      } else {
        request_id = create_request_id();
        fix::OrderCancelReplaceRequest request {
          .cl_ord_id = request_id,
          .orig_cl_ord_id = order_mapping.exchange_order_id(),
          .side = core::fix::map(order_mapping.side()),
          .order_qty = modify_order.quantity,
          .ord_type = core::fix::map(order_mapping.order_type()),
          .price = modify_order.price,
          .symbol = order_mapping.symbol(),
          .transact_time = order_mapping.update_time(),
        };
        try {
          send(request);
          order_mapping.update_request(request_id, type);
        } catch (std::exception& e) {
          LOG(WARNING)("Exception: what=\"{}\"", e.what());
          error = Error::NETWORK_ERROR;
        }
      }
    }
  }
  OrderAck order_ack {
    .account = modify_order.account,
    .order_id = modify_order.order_id,
    .type = type,
    .origin = origin,
    .status = (error == Error::NONE)
      ? RequestStatus::FORWARDED
      : RequestStatus::REJECTED,
    .error = error,
    .text = std::string_view(),
    .gateway_order_id = gateway_order_id,
    .external_order_id = external_order_id,
    .request_id = request_id,
  };
  _dispatcher.enqueue(
      message_info.source,
      order_ack,
      message_info.source_receive_time,
      message_info.origin_create_time,
      true);
}

void Gateway::operator()(const CancelOrderEvent& event) {
  auto& message_info = event.message_info;
  auto& cancel_order = event.cancel_order;
  constexpr auto type = RequestType::CANCEL_ORDER;
  constexpr auto origin = Origin::GATEWAY;
  auto error = Error::NONE;
  decltype(OrderAck::gateway_order_id) gateway_order_id = {};
  decltype(OrderAck::external_order_id) external_order_id = {};
  std::string request_id;
  if (unlikely(_gateway_status != GatewayStatus::READY)) {
    error = Error::GATEWAY_NOT_READY;
  } else if (unlikely(
        cancel_order.account.compare(_account) != 0)) {
    error = Error::INVALID_ACCOUNT;
  } else {
    auto key = OrderMapping::key(
        message_info.source,
        cancel_order.order_id);
    auto iter = _order_mapping.find(key);
    if (unlikely(iter == _order_mapping.end())) {
      error = Error::UNKNOWN_ORDER_ID;
    } else {
      auto& order_mapping = (*iter).second;
      gateway_order_id = order_mapping.gateway_order_id();
      external_order_id = order_mapping.exchange_order_id();
      if (unlikely(order_mapping.ready() == false)) {
        error = Error::UNKNOWN_EXCHANGE_ORDER_ID;
      } else {
        request_id = create_request_id();
        fix::OrderCancelRequest request {
          .cl_ord_id = request_id,
          .orig_cl_ord_id = order_mapping.exchange_order_id(),
        };
        try {
          send(request);
          order_mapping.update_request(request_id, type);
        } catch (std::exception& e) {
          LOG(WARNING)("Exception: what=\"{}\"", e.what());
          error = Error::NETWORK_ERROR;
        }
      }
    }
  }
  OrderAck order_ack {
    .account = cancel_order.account,
    .order_id = cancel_order.order_id,
    .type = type,
    .origin = origin,
    .status = (error == Error::NONE)
      ? RequestStatus::FORWARDED
      : RequestStatus::REJECTED,
    .error = error,
    .text = std::string_view(),
    .gateway_order_id = gateway_order_id,
    .external_order_id = external_order_id,
    .request_id = request_id,
  };
  _dispatcher.enqueue(
      message_info.source,
      order_ack,
      message_info.source_receive_time,
      message_info.origin_create_time,
      true);
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
  LOG(INFO)(FIX_PREFIX
      "sending logon request (username=\"{}\")...", _access_key);
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
  VLOG(1)(FIX_PREFIX
      "event(header={}, execution_report={})",
      header,
      execution_report);
  check(header);

  // download begin?
  switch (execution_report.mass_status_req_type) {
    case core::fix::MassStatusReqType::ORDERS:
      assert(_gateway_status == GatewayStatus::DOWNLOADING);
      _download_execution_reports = execution_report.tot_num_reports;
      if (_download_execution_reports == 0)
        check_download();
      return;
    default:
      break;
  }

  // order mapping
  auto iter = find_order_mapping(
      execution_report.cl_ord_id,
      execution_report.orig_cl_ord_id);
  if (unlikely(iter == _order_mapping.end())) {
    if (execution_report.deribit_label.empty()) {
      LOG(WARNING)("*** EXTERNAL ORDER ***");
    } else {
      iter = create_order_mapping(execution_report);
    }
  }
  if (iter != _order_mapping.end()) {
    auto& order_mapping = (*iter).second;

    // FIXME(thraneh): DEBUG
    if (unlikely(
          order_mapping.validate(execution_report) == false)) {
      LOG(WARNING)("*** SOMETHING WRONG ***");
    } else {
      constexpr auto origin = Origin::EXCHANGE;
      auto status = RequestStatus::UNDEFINED;
      auto error = fix::map_error(execution_report.text);
      bool order_update = true;
      switch (execution_report.exec_type) {
        case core::fix::ExecType::REJECTED: {
          switch (order_mapping.request()) {
            case RequestType::UNDEFINED:
              LOG(WARNING)("*** EXTERNAL ACTION ***");
              break;
            case RequestType::CREATE_ORDER:
              order_update = false;
              [[ fallthrough ]];
            case RequestType::MODIFY_ORDER:
            case RequestType::CANCEL_ORDER:
              status = RequestStatus::REJECTED;
              break;
          }
          break;
        }
        case core::fix::ExecType::CANCELED: {
          switch (order_mapping.request()) {
            case RequestType::UNDEFINED:
              LOG(WARNING)("*** EXTERNAL ACTION ***");
              break;
            case RequestType::CREATE_ORDER:
            case RequestType::MODIFY_ORDER:
              LOG(FATAL)("DEBUG: UNEXPECTED");
              break;
            case RequestType::CANCEL_ORDER:
              status = RequestStatus::ACCEPTED;
              order_update = false;
              break;
          }
          break;
        }
        case core::fix::ExecType::ORDER_STATUS:
          switch (execution_report.ord_status) {
            case core::fix::OrdStatus::NEW: {
              switch (order_mapping.request()) {
                case RequestType::UNDEFINED:
                  switch (_download) {
                    case Download::NONE:
                      LOG(WARNING)("*** EXTERNAL ACTION ***");
                      break;
                    case Download::ORDERS:
                      break;
                    default:
                      LOG(FATAL)("DEBUG: UNEXPECTED");
                  }
                  break;
                case RequestType::CREATE_ORDER:
                case RequestType::MODIFY_ORDER:
                  status = RequestStatus::ACCEPTED;
                  break;
                case RequestType::CANCEL_ORDER:
                  LOG(FATAL)("DEBUG: UNEXPECTED");
                  break;
              }
              break;
            }
            case core::fix::OrdStatus::PARTIALLY_FILLED:
            case core::fix::OrdStatus::FILLED:
              break;
            case core::fix::OrdStatus::CANCELED:
              // TODO(thraneh): how to signal external action
              // -- we need to add an "expectation" into order_mapping?
              break;
            default:
              LOG(FATAL)("DEBUG: UNEXPECTED");
              break;
          }
          break;
        default:
          LOG(FATAL)("DEBUG: UNEXPECTED");
          break;
      }
      if (status != RequestStatus::UNDEFINED) {
        OrderAck order_ack {
          .account = _account,
          .order_id = order_mapping.user_order_id(),
          .type = order_mapping.request(),
          .origin = origin,
          .status = (error == Error::NONE)
            ? RequestStatus::ACCEPTED
            : RequestStatus::REJECTED,
          .error = error,
          .text = execution_report.text,
          .gateway_order_id = order_mapping.gateway_order_id(),
          .external_order_id = order_mapping.exchange_order_id(),
          .request_id = execution_report.orig_cl_ord_id,
        };
        enqueue(
            order_mapping.user_id(),
            order_ack,
            order_update == false);  // only "last" if no order_update
        order_mapping.reset_request();
      }
      if (order_update) {
        for (size_t i = 0; i < execution_report.fills_grp.length; ++i) {
          const auto& fills = execution_report.fills_grp.items[i];
          // TODO(thraneh): map fill_exec_id <-> local_trade_id ???
          auto trade_id = create_trade_id();
          TradeUpdate trade_update {
            .account = _account,
            .trade_id = trade_id,
            .order_id = order_mapping.user_order_id(),
            .exchange = FLAGS_exchange,
            .symbol = execution_report.symbol,
            .side = order_mapping.side(),
            .quantity = fills.fill_qty,
            .price = fills.fill_px,
            .position_effect = PositionEffect::UNDEFINED,
            .order_template = std::string(),
            .create_time_utc = execution_report.transact_time,
            .update_time_utc = execution_report.transact_time,
            .gateway_order_id = order_mapping.gateway_order_id(),
            .gateway_trade_id = trade_id,
            .external_order_id = order_mapping.exchange_order_id(),
            .external_trade_id = fills.fill_exec_id,
          };
          enqueue(
              order_mapping.user_id(),
              trade_update,
              false);
        }
        OrderUpdate order_update {
          .account = _account,
          .order_id = order_mapping.user_order_id(),
          .exchange = FLAGS_exchange,
          .symbol = execution_report.symbol,
          .status = core::fix::map(execution_report.ord_status),
          .side = order_mapping.side(),
          .price = execution_report.price,
          .remaining_quantity = execution_report.leaves_qty,
          .traded_quantity = execution_report.cum_qty,
          .position_effect = PositionEffect::UNDEFINED,
          .order_template = std::string(),
          .create_time_utc = order_mapping.create_time(),
          .update_time_utc = order_mapping.update_time(),
          .commissions = execution_report.commission,
          .gateway_order_id = order_mapping.gateway_order_id(),
          .external_order_id = order_mapping.exchange_order_id(),
        };
        enqueue(
            order_mapping.user_id(),
            order_update,
            true);

      } else {
        LOG_IF(FATAL, execution_report.fills_grp.length > 0)(
            "DEBUG: UNEXPECTED");
      }
    }

  } else {
    // TODO(thraneh): process fills? --> maintain positions
  }

  // download end?
  if (_download_execution_reports &&
      0 == --_download_execution_reports) {
    check_download();
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Heartbeat& heartbeat) {
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  VLOG(3)(FIX_PREFIX
      "event(header={}, heartbeat={})",
      header,
      heartbeat);
  check(header);
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  if (heartbeat.test_req_id.empty() == false) {
    auto send_time = core::from_chars<uint64_t>(heartbeat.test_req_id);
    auto latency =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          now - decltype(now){send_time}) / 2;  // 1-way
    _fix_latency.update(latency.count());
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Logon& logon) {
  VLOG(1)(FIX_PREFIX
      "event(header={}, logon={})",
      header,
      logon);
  check(header);
  assert(_gateway_status == GatewayStatus::LOGIN_SENT);
  LOG(INFO)(FIX_PREFIX "logon COMPLETED");
  begin_download();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Logout& logout) {
  LOG(WARNING)(FIX_PREFIX
      "event(header={}, logout={})",
      header,
      logout);
  check(header);
  assert(_gateway_status == GatewayStatus::READY);
  update(GatewayStatus::LOGGED_OUT);
  // note! mandated, must send a logout response
  fix::Logout response = {
    .text = LOGOUT_RESPONSE,
  };
  send(response);
  LOG(INFO)(FIX_PREFIX "closing connection");
  _fix->stop();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
  core::metrics::profile(_market_data_incremental_refresh, [&]() {
    VLOG(3)(FIX_PREFIX
        "event(header={}, market_data_incremental_refresh={})",
        header,
        market_data_incremental_refresh);
    check(header);
    assert(_gateway_status == GatewayStatus::READY);
    // weirdness -- told them and they would investigate
    if (unlikely(FLAGS_silence_empty_messages &&
          market_data_incremental_refresh.md_inc_grp.length == 0))
      return;

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
          VLOG(4)(FIX_PREFIX "unsupported: {}", item);
          break;
        default:
          LOG(WARNING)(FIX_PREFIX "unsupported: {}", item);
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
  LOG(WARNING)(FIX_PREFIX
      "event(header={}, market_data_request_reject={})",
      header,
      market_data_request_reject);
  check(header);
  assert(_gateway_status == GatewayStatus::READY);
  LOG(FATAL)("Unexpected -- now what?");  // FIXME(thraneh): ...
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
  VLOG(3)(FIX_PREFIX
      "event(header={}, market_data_snapshot_full_refresh={})",
      header,
      market_data_snapshot_full_refresh);
  check(header);
  assert(_gateway_status == GatewayStatus::READY);
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
  // if (bid_length == 0 && ask_length == 0) return;  // TODO(thraneh): check roq-server support
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
  VLOG(1)(FIX_PREFIX
      "event(header={}, order_cancel_reject={})",
      header,
      order_cancel_reject);
  check(header);
  assert(_gateway_status == GatewayStatus::READY);
  auto iter = find_order_mapping(
      order_cancel_reject.cl_ord_id,
      order_cancel_reject.orig_cl_ord_id);
  if (iter == _order_mapping.end()) {
    LOG(WARNING)("*** EXTERNAL ORDER ***");
  } else {
    auto& order_mapping = (*iter).second;
    constexpr auto origin = Origin::EXCHANGE;
    auto status = RequestStatus::UNDEFINED;
    auto error = fix::map_error(order_cancel_reject.text);
    switch (order_mapping.request()) {
      case RequestType::UNDEFINED:
        LOG(WARNING)("*** EXTERNAL ACTION ***");
        break;
      case RequestType::CREATE_ORDER:
      case RequestType::MODIFY_ORDER:
        LOG(FATAL)("DEBUG: UNEXPECTED");
        break;
      case RequestType::CANCEL_ORDER:
        status = RequestStatus::REJECTED;
        break;
    }
    if (status != RequestStatus::UNDEFINED) {
      OrderAck order_ack {
        .account = _account,
        .order_id = order_mapping.user_order_id(),
        .type = order_mapping.request(),
        .origin = origin,
        .status = status,
        .error = error,
        .text = order_cancel_reject.text,
        .gateway_order_id = order_mapping.gateway_order_id(),
        .external_order_id = order_mapping.exchange_order_id(),
        .request_id = order_cancel_reject.orig_cl_ord_id,
      };
      enqueue(
          order_mapping.user_id(),
          order_ack,
          true);
      order_mapping.reset_request();
    }
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::PositionReport& position_report) {
  VLOG(2)(FIX_PREFIX
      "event(header={}, position_report={})",
      header,
      position_report);
  check(header);
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  switch (position_report.pos_req_result) {
    case core::fix::PosReqResult::VALID:
      switch (position_report.pos_req_type) {
        case core::fix::PosReqType::POSITIONS: {
          size_t position_count = 0;
          for (size_t i = 0; i < position_report.positions.length; ++i) {
            auto& position = position_report.positions.items[i];
            PositionUpdate buy {
              .account = _account,
              .exchange = FLAGS_exchange,
              .symbol = position.symbol,
              .side = Side::BUY,
              .position = position.long_qty,
              .last_trade_id = 0,
              .position_cost = 0.0,
              .position_yesterday = 0.0,
              .position_cost_yesterday = 0.0,
            };
            PositionUpdate sell {
              .account = _account,
              .exchange = FLAGS_exchange,
              .symbol = position.symbol,
              .side = Side::SELL,
              .position = position.short_qty,
              .last_trade_id = 0,
              .position_cost = 0.0,
              .position_yesterday = 0.0,
              .position_cost_yesterday = 0.0,
            };
            enqueue(buy, false);
            enqueue(sell, true);
            ++position_count;
          }
          VLOG(1)(
              "- positions: {} (/{})",
              position_count,
              position_report.positions.length);
          break;
        }
        default:
          LOG(FATAL)("DEBUG: UNEXPECTED");
          break;
      }
      break;
    default:
      LOG(FATAL)("DEBUG: UNEXPECTED");
      break;
  }

  check_download();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Reject& reject) {
  LOG(WARNING)(FIX_PREFIX
      "event(header={}, reject={})",
      header,
      reject);
  check(header);
  assert(_gateway_status != GatewayStatus::DISCONNECTED);

  auto request_id = fmt::format(  // FIXME(thraneh): this is *wrong*
      "roq:{:06}", reject.ref_seq_num);
  // FIXME(thraneh): this lookup doesn't *just* manage cl_ord_id
  auto iter = find_order_mapping(request_id, std::string_view());
  if (iter != _order_mapping.end())  {
    auto& order_mapping = (*iter).second;
    auto error = fix::map_error(reject.text);
    OrderAck order_ack {
      .account = _account,
      .order_id = order_mapping.user_order_id(),
      .type = RequestType::CREATE_ORDER,  // FIXME(thraneh): from order_mapping
      .origin = Origin::EXCHANGE,
      .status = RequestStatus::REJECTED,
      .error = error,
      .text = reject.text,
      .gateway_order_id = order_mapping.gateway_order_id(),
      .external_order_id = order_mapping.exchange_order_id(),
      .request_id = request_id,
    };
    auto now = core::get_system_clock();
    _dispatcher.enqueue(
        order_mapping.user_id(),
        order_ack,
        now,
        now,
        true);
  } else {
    LOG(INFO)(FIX_PREFIX "closing connection");
    _fix->stop();
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::ResendRequest& resend_request) {
  LOG(WARNING)(FIX_PREFIX
      "event(header={}, resend_request={})",
      header,
      resend_request);
  check(header);
  fix::Reject reject = {
    .ref_seq_num = header.msg_seq_num,
    .ref_tag_id = 0,
    .ref_msg_type = header.msg_type_raw,
    .text = RESEND_NOT_SUPPORTED,
  };
  send(reject);
  LOG(INFO)(FIX_PREFIX "closing connection");
  _fix->stop();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::SecurityList& security_list) {
  VLOG(2)(FIX_PREFIX
      "event(header={}, security_list={})",
      header,
      security_list);
  check(header);
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  _currencies.clear();
  if (security_list.instruments.length) {
    assert(_symbols.empty());
    size_t security_count = 0;
    _symbols.reserve(security_list.instruments.length);  // note! alloc
    for (size_t i = 0; i < security_list.instruments.length; ++i) {
      auto& instrument = security_list.instruments.items[i];
      // note!
      //   USD will cause a Reject
      //   using commission currency because it requires funding
      if (instrument.comm_currency.empty() == false)
        _currencies.emplace(instrument.comm_currency);
      if (discard_symbol(instrument.symbol))
        continue;
      _symbols.emplace_back(instrument.symbol);
      ReferenceData reference_data = {
        .exchange = FLAGS_exchange,
        .symbol = instrument.symbol,
        .security_type = fix::map_security_type(instrument.security_type),
        .currency = instrument.currency,
        .settlement_currency = instrument.settl_currency,
        .commission_currency = instrument.comm_currency,
        .tick_size = instrument.min_price_increment,
        .limit_up = std::numeric_limits<double>::quiet_NaN(),
        .limit_down = std::numeric_limits<double>::quiet_NaN(),
        .multiplier = instrument.contract_multiplier,
        .min_trade_vol = instrument.min_trade_vol,
        .option_type = core::fix::map(instrument.put_or_call),
        .strike_currency = instrument.strike_currency,
        .strike_price = instrument.strike_price,
      };
      enqueue(reference_data, false);
      // note! we receive no information about the trading status
      MarketStatus market_status = {
        .exchange = FLAGS_exchange,
        .symbol = instrument.symbol,
        .trading_status = TradingStatus::OPEN,  // TODO(thraneh): missing
      };
      enqueue(market_status, true);
      ++security_count;
    }
    VLOG(1)(
        "- securities: {} (/{})",
        security_count,
        security_list.instruments.length);
  }
  check_download();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::TestRequest& test_request) {
  VLOG(1)(FIX_PREFIX
      "event(header={}, test_request={})",
      header,
      test_request);
  check(header);
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  fix::Heartbeat heartbeat = {
    .test_req_id = test_request.test_req_id,
  };
  send(heartbeat);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::UserResponse& user_response) {
  VLOG(1)(FIX_PREFIX
      "event(header={}, user_response={})",
      header,
      user_response);
  check(header);
  FundsUpdate funds_update {
    .account = _account,
    .currency = user_response.currency,
    .balance = user_response.deribit_user_balance,
    .hold = double{0.0},
  };
  enqueue(funds_update, true);
  if (_download == Download::USER) {
    if (_download_users && 0 == --_download_users)
      check_download();
  }
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
  LOG(INFO)("Update: gateway_status={}", _gateway_status);
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
  LOG(INFO)(FIX_PREFIX "download securities...");
  auto security_req_id = create_request_id();
  fix::SecurityListRequest security_list_request = {
    .security_req_id = security_req_id,
  };
  send(security_list_request);
  _download = Download::SECURITIES;
}

void Gateway::download_positions() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)(FIX_PREFIX "download positions...");
  auto pos_req_id = create_request_id();
  fix::RequestForPositions request_for_positions = {
    .pos_req_id = pos_req_id,
    .pos_req_type = core::fix::PosReqType::POSITIONS,
  };
  send(request_for_positions);
  _download = Download::POSITIONS;
}

void Gateway::download_orders() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)(FIX_PREFIX "download orders...");
  auto mass_status_req_id = create_request_id();
  fix::OrderMassStatusRequest order_mass_status_request = {
    .mass_status_req_id = mass_status_req_id,
    .mass_status_req_type = core::fix::MassStatusReqType::ORDERS,
  };
  send(order_mass_status_request);
  _download = Download::ORDERS;
}

void Gateway::download_user() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)(FIX_PREFIX "download user...");
  auto user_request_id = create_request_id();
  // FIXME(thraneh): documentation refers to SecurityList
  assert(_currencies.empty() == false);
  _download_users = _currencies.size();
  for (auto& currency : _currencies) {
    fix::UserRequest user_request_btc = {
      .user_request_id = user_request_id,
      .username = _access_key,
      .currency = static_cast<std::string_view>(currency),
    };
    send(user_request_btc);
  }
  _download = Download::USER;
}

void Gateway::reset() {
  _msg_seq_num = 0;
  _download = Download::NONE;
  _symbols.clear();
  _their_msg_seq_num = 0;
}

void Gateway::subscribe_market_data() {
  assert(_gateway_status == GatewayStatus::READY);
  if (_symbols.empty()) {
    LOG(WARNING)("Can't subscribe market data, reason: NO SYMBOLS");
    return;
  }
  LOG(INFO)("Subscribe market data");
  auto md_req_id = create_request_id();
  // FIXME(thraneh): simplify this
  if (FLAGS_batch_subscribe) {
    std::vector<std::string_view> symbols(FLAGS_max_batch_size);
    size_t j = 0;
    for (size_t i = 0; i < _symbols.size(); ++i) {
      symbols[j++] = _symbols[i];
      if (j == symbols.size()) {
        fix::MarketDataRequest market_data_request = {
          .md_req_id = md_req_id,
          .symbol = {},
          .symbols = symbols,
        };
        send(market_data_request);
        j = 0;
      }
    }
    if (j > 0) {
      fix::MarketDataRequest market_data_request = {
        .md_req_id = md_req_id,
        .symbol = {},
        .symbols = symbols,
      };
      send(market_data_request);
    }
  } else {
    for (auto& symbol : _symbols) {
      auto md_req_id = create_request_id();
      fix::MarketDataRequest market_data_request = {
        .md_req_id = md_req_id,
        .symbol = symbol,
        .symbols = {},
      };
      send(market_data_request);
    }
  }
}

std::string Gateway::create_request_id() {
  return fmt::format(  // FIXME(thraneh): use charconv
      "roq:{:06}",
      ++_request_id);
}

decltype(Gateway::_order_mapping)::iterator
Gateway::find_order_mapping(
    const std::string_view& cl_ord_id,
    const std::string_view& orig_cl_ord_id) {
  auto iter = _order_lookup.find(cl_ord_id);
  if (unlikely(iter == _order_lookup.end())) {
    iter = _order_lookup.find(orig_cl_ord_id);
    if (unlikely(iter == _order_lookup.end())) {
      return _order_mapping.end();
    } else {
      // replace orig_cl_ord_id with cl_ord_id
      auto key = (*iter).second;
      _order_lookup.erase(iter);
      iter = _order_lookup.emplace(std::string(cl_ord_id), key).first;
    }
  }
  return _order_mapping.find((*iter).second);
}

decltype(Gateway::_order_mapping)::iterator
Gateway::create_order_mapping(
    const fix::ExecutionReport& execution_report) {
  try {
    OrderMapping order_mapping(
        execution_report,
        execution_report.deribit_label);
    auto key = order_mapping.key();
    auto iter = _order_mapping.emplace(
        key,
        std::move(order_mapping)).first;
    _order_lookup.emplace(
        std::string(execution_report.cl_ord_id),
        key);
    return iter;
  } catch (core::oms::InvalidUserCustom&) {
    LOG(WARNING)("*** INVALID USER_CUSTOM ***");
    return _order_mapping.end();
  }
}

template <typename T>
inline void Gateway::send(const T& event) {
  send(event, core::get_realtime_clock());
}

template <typename T>
void Gateway::send(
    const T& event,
    const std::chrono::nanoseconds sending_time) {
  VLOG(1)(FIX_PREFIX "event={}", event);
  assert(static_cast<bool>(_fix));  // a check missing somehwere else
  if (static_cast<bool>(_fix) == false) return;  // FIXME(thraneh): DEBUG
  auto message = event.encode(
      _encode_buffer,
      _msg_seq_num,
      sending_time);
  // message.print();  // DEBUG
  _fix->send(message);
}

void Gateway::check(const core::fix::header_t& header) {
  auto current = header.msg_seq_num;
  auto expected = _their_msg_seq_num + 1;
  if (unlikely(current != expected)) {
    if (expected < current) {
      LOG(WARNING)(FIX_PREFIX
          "*** SEQUENCE GAP *** current={} previous={} distance={}",
          current,
          _their_msg_seq_num,
          current - _their_msg_seq_num);
    } else {
      LOG(WARNING)(FIX_PREFIX
          "*** SEQUENCE REPLAY *** current={} previous={} distance={}",
          current,
          _their_msg_seq_num,
          _their_msg_seq_num - current);
    }
  }
  _their_msg_seq_num = current;
}

}  // namespace deribit
}  // namespace roq
