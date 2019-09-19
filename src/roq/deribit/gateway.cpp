/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <limits>

#include "roq/logging.h"

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

namespace roq {
namespace deribit {

namespace {
constexpr auto DECODE_BUFFER_SIZE = size_t{1048576};  // FIXME(thraneh): flag
constexpr auto ENCODE_BUFFER_SIZE = size_t{4096};  // FIXME(thraneh): flag
constexpr auto PING_FREQUENCY = std::chrono::seconds{10};
constexpr auto CANCEL_ON_DISCONNECT = true;
constexpr auto EXCHANGE = "DERIBIT";
constexpr auto ACCOUNT = "A1";
constexpr auto SYMBOL = "BTC-27SEP19";
constexpr auto MAX_DEPTH = size_t{256};
}  // namespace

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const conf::Config& config,
    const core::URI& ws_uri,
    const core::URI& fix_uri)
    : _dispatcher(dispatcher),
      _dns_base(_base, true),
      _timer(_base, EV_PERSIST, [this]() { on_timer(); }),
      _ssl_connection(_ssl_context),
      _buffer_event(_base, _ssl_connection),
      _fix(
          *this,
          _ssl_context,
          _base,
          _dns_base,
          fix_uri),
      _decode_buffer(DECODE_BUFFER_SIZE),
      _encode_buffer(ENCODE_BUFFER_SIZE),
      _access_key(config.get_access_key()),
      _access_secret(config.get_access_secret()) {
}

void Gateway::on(const StartEvent& event) {
  LOG(INFO) << "Starting the gateway event loop...";
  _thread = std::thread([this]() { run(); });
}

void Gateway::on(const StopEvent& event) {
  LOG(INFO) << "Stopping the gateway event loop...";
  _stop.store(true, std::memory_order_release);
  if (_thread.joinable())
    _thread.join();
  LOG(INFO) << "The gateway event loop has stopped";
}

void Gateway::on(const TimerEvent& event) {
}

void Gateway::on(const ConnectionStatusEvent& event) {
}

void Gateway::on(const CreateOrderEvent& event) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::on(const ModifyOrderEvent& event) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::on(const CancelOrderEvent& event) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::write(Metrics& metrics) const {
}

void Gateway::run() {
  LOG(INFO) << "Gateway event loop has started";
  try {
    initialize_thread();
    _timer.add(std::chrono::milliseconds{100});

    _fix.start();  // FIXME(thraneh): move to Controller

    _base.loop(EVLOOP_NO_EXIT_ON_EMPTY);
  } catch (std::exception& e) {
    LOG(FATAL) << "Unhandled exception, what=\"" << e.what() << "\"";
  } catch (...) {
    LOG(FATAL) << "Unhandled exception";
  }
  LOG(INFO) << "Gateway event loop has finished";
}

void Gateway::initialize_thread() {
  // TODO(thraneh): affinity
}

void Gateway::on_timer() {
  if (_stop.load(std::memory_order_acquire)) {
    _base.loopbreak();
    return;
  }
  auto now = core::get_time();
  if (now < _next_update)
    return;
  _next_update = now + PING_FREQUENCY;
  auto test_req_id = fmt::format(
      "{}",
      core::get_system_clock().count());
  fix::TestRequest test_request {
    .test_req_id = test_req_id,
  };
  send(test_request);
}

void Gateway::on_fix_connected() {
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::LOGIN_SENT,
  };
  enqueue(order_manager_status, true);
  auto sending_time = core::get_realtime_clock();
  auto raw_data = Random::create_raw_data(sending_time);
  auto password = Random::create_password(raw_data, _access_secret);
  fix::Logon logon = {
    .heart_bt_int = PING_FREQUENCY.count(),
    .raw_data = raw_data,
    .username = _access_key,
    .password = password,
    .deribit_cancel_on_disconnect = CANCEL_ON_DISCONNECT,
  };
  send(logon, sending_time);
}

void Gateway::on_fix_disconnected() {
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::LOGGED_OUT,
  };
  enqueue(order_manager_status, true);
}

void Gateway::operator()(
    const fix::ExecutionReport& execution_report,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "execution_report={}, seq_num={}",
      execution_report,
      seq_num);
  switch (execution_report.exec_type) {
    case core::fix::ExecType::ORDER_STATUS:
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
      break;
    default:
      break;
  }
}

void Gateway::operator()(
    const fix::Heartbeat& heartbeat,
    uint64_t seq_num) {
  VLOG(3) << fmt::format(
      "heartbeat={}, seq_num={}",
      heartbeat,
      seq_num);
  if (!heartbeat.test_req_id.empty()) {
    auto tmp = core::charconv::from_string<uint64_t>(
        heartbeat.test_req_id);
    std::chrono::nanoseconds send_time{tmp};
    auto latency = std::chrono::duration_cast<
      std::chrono::microseconds>(core::get_system_clock() - send_time);
    LOG(INFO) << fmt::format("LATENCY={}", latency);
  }
}

void Gateway::operator()(
    const fix::Logon& logon,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "logon={}, seq_num={}",
      logon,
      seq_num);
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::DOWNLOADING,
  };
  enqueue(order_manager_status, true);

  fix::SecurityListRequest security_list_request = {
    .security_req_id = "roq-sec-001",
  };
  send(security_list_request);

  fix::MarketDataRequest market_data_request = {
    .md_req_id = "roq-mkt-002",
    .symbol = SYMBOL,
  };
  send(market_data_request);

  fix::RequestForPositions request_for_positions = {
    .pos_req_id = "roq-pos-003",
    .pos_req_type = core::fix::PosReqType::POSITIONS,
  };
  send(request_for_positions);

  fix::UserRequest user_request = {
    .user_request_id = "roq-usr-004",
    .username = _access_key,
  };
  send(user_request);

  fix::OrderMassStatusRequest order_mass_status_request = {
      "roq-oms-005",
      core::fix::MassStatusReqType::ORDERS,
  };
  send(order_mass_status_request);

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
}

void Gateway::operator()(
    const fix::Logout& logout,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "logout={}, seq_num={}",
      logout,
      seq_num);
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::LOGGED_OUT,
  };
  enqueue(order_manager_status, true);

  fix::Logout logout_response = {
    .text = "i'm done",
  };
  send(logout_response);
  // TODO(thraneh): ...now what?
}

void Gateway::operator()(
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh,
    uint64_t seq_num) {
  VLOG(3) << fmt::format(
      "market_data_incremental_refresh={}, seq_num={}",
      market_data_incremental_refresh,
      seq_num);
  std::string FIXME_SYMBOL(
      market_data_incremental_refresh.symbol.data(),
      market_data_incremental_refresh.symbol.length());
  MBPUpdate bid[MAX_DEPTH];
  MBPUpdate ask[MAX_DEPTH];
  size_t bid_length = 0, ask_length = 0;
  std::chrono::nanoseconds exchange_time_utc = {};
  auto& md_inc_grp = market_data_incremental_refresh.md_inc_grp;
  for (size_t i = 0; i < md_inc_grp.length; ++i) {
    auto& item = md_inc_grp.items[i];
    if (item.md_entry_date > exchange_time_utc)
      exchange_time_utc = item.md_entry_date;
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        new (&bid[bid_length++]) MBPUpdate {
          .price = item.md_entry_px,
          .quantity = item.md_entry_size,
          .action = core::fix::map(item.md_update_action),
        };
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        new (&ask[ask_length++]) MBPUpdate {
          .price = item.md_entry_px,
          .quantity = item.md_entry_size,
          .action = core::fix::map(item.md_update_action),
        };
        break;
      }
      case core::fix::MDEntryType::TRADE: {
        TradeSummary trade_summary = {
          .exchange = EXCHANGE,
          .symbol = FIXME_SYMBOL.c_str(),  // FIXME(thraneh): use string_view
          .price = item.md_entry_px,
          .volume = item.md_entry_size,
          .turnover = std::numeric_limits<double>::quiet_NaN(),
          .side = core::fix::map(item.side),
          .exchange_time_utc = exchange_time_utc,
        };
        enqueue(trade_summary, true);  // FIXME(thraneh): *not* always last
        break;
      }
      default:
        LOG(WARNING) << fmt::format("Unsupported: {}", item);
        break;
    }
  }
  if (bid_length > 0 || ask_length > 0) {
    MarketByPrice market_by_price = {
      .exchange = EXCHANGE,
      .symbol = FIXME_SYMBOL.c_str(),  // FIXME(thraneh): use string_view
      .bid_length = bid_length,
      .bid = bid,
      .ask_length = ask_length,
      .ask = ask,
      .snapshot = false,
      .exchange_time_utc = exchange_time_utc,
    };
    enqueue(market_by_price, true);
  }
}

void Gateway::operator()(
    const fix::MarketDataRequestReject& market_data_request_reject,
    uint64_t seq_num) {
  LOG(WARNING) << fmt::format(
      "market_data_request_reject={}, seq_num={}",
      market_data_request_reject,
      seq_num);
}

void Gateway::operator()(
    const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh,
    uint64_t seq_num) {
  VLOG(3) << fmt::format(
      "market_data_snapshot_full_refresh={}, seq_num={}",
      market_data_snapshot_full_refresh,
      seq_num);
  MBPUpdate bid[MAX_DEPTH];
  MBPUpdate ask[MAX_DEPTH];
  std::string FIXME_SYMBOL(
      market_data_snapshot_full_refresh.symbol.data(),
      market_data_snapshot_full_refresh.symbol.length());
  MarketByPrice market_by_price = {
    .exchange = EXCHANGE,
    .symbol = FIXME_SYMBOL.c_str(),  // FIXME(thraneh): use string_view
    .bid_length = 0,
    .bid = bid,
    .ask_length = 0,
    .ask = ask,
    .snapshot = true,
    .exchange_time_utc = {},
  };
  auto& md_full_grp = market_data_snapshot_full_refresh.md_full_grp;
  for (size_t i = 0; i < md_full_grp.length; ++i) {
    auto& item = md_full_grp.items[i];
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        new (&bid[market_by_price.bid_length++]) MBPUpdate {
          .price = item.md_entry_px,
          .quantity = item.md_entry_size,
          .action = UpdateAction::NEW,
        };
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        new (&ask[market_by_price.ask_length++]) MBPUpdate {
          .price = item.md_entry_px,
          .quantity = item.md_entry_size,
          .action = UpdateAction::NEW,
        };
        break;
      }
      default:
        break;
    }
  }
  enqueue(market_by_price, true);
}

void Gateway::operator()(
    const fix::OrderCancelReject& order_cancel_reject,
    uint64_t seq_num) {
  LOG(WARNING) << fmt::format(
      "order_cancel_reject={}, seq_num={}",
      order_cancel_reject,
      seq_num);
}

void Gateway::operator()(
    const fix::PositionReport& position_report,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "position_report={}, seq_num={}",
      position_report,
      seq_num);
}

void Gateway::operator()(
    const fix::Reject& reject,
    uint64_t seq_num) {
  LOG(WARNING) << fmt::format(
      "reject={}, seq_num={}",
      reject,
      seq_num);
}

void Gateway::operator()(
    const fix::ResendRequest& resend_request,
    uint64_t seq_num) {
  LOG(WARNING) << fmt::format(
      "resend_request={}, seq_num={}",
      resend_request,
      seq_num);
  /*
  fix::Reject reject = {
    .ref_seq_num = header.msg_seq_num,
    .ref_tag_id = 0,
    .ref.msg_type = header.msg_type,
    .text = "resend_not_supported",
  };
  send(reject);
  */
}

void Gateway::operator()(const fix::SecurityList& security_list,
    uint64_t seq_num) {
  VLOG(3) << fmt::format(
      "security_list={}, seq_num={}",
      security_list,
      seq_num);
  for (size_t i = 0; i < security_list.instruments.length; ++i) {
    auto& instrument = security_list.instruments.items[i];
    std::string FIXME_SYMBOL(instrument.symbol.data(), instrument.symbol.length());
    ReferenceData reference_data = {
      .exchange = EXCHANGE,
      .symbol = FIXME_SYMBOL.c_str(),  // FIXME(thraneh): use string_view
      .tick_size = instrument.min_price_increment,
      .limit_up = std::numeric_limits<double>::quiet_NaN(),
      .limit_down = std::numeric_limits<double>::quiet_NaN(),
      .multiplier = instrument.contract_multiplier ,
    };
    enqueue(reference_data, true);
  }
}

void Gateway::operator()(const fix::TestRequest& test_request,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "test_request={}, seq_num={}",
      test_request,
      seq_num);
  fix::Heartbeat heartbeat = {
    .test_req_id = test_request.test_req_id,
  };
  send(heartbeat);
}

void Gateway::operator()(const fix::UserResponse& user_response,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "user_response={}, seq_num={}",
      user_response,
      seq_num);
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::READY,
  };
  enqueue(order_manager_status, true);
}

}  // namespace deribit
}  // namespace roq
