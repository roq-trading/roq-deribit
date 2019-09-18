/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/controller.h"

#include "roq/logging.h"

#include "roq/core/clock.h"

#include "roq/core/fix/utils.h"

#include "roq/deribit/gateway.h"
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

Controller::Controller(
    Gateway& gateway,
    const std::string_view& access_key,
    const std::string_view& access_secret)
    : _gateway(gateway),
      _decode_buffer(DECODE_BUFFER_SIZE),
      _encode_buffer(ENCODE_BUFFER_SIZE),
      _access_key(access_key),
      _access_secret(access_secret) {
}

void Controller::on_timer() {
  auto now = core::get_time();
  if (now < _next_update)
    return;
  _next_update = now + PING_FREQUENCY;
  auto test_req_id = fmt::format(
      "{}",
      core::get_system_clock().count());
  auto message = fix::TestRequest::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      test_req_id);
  _gateway.fix().send(message);
}

// rest api:

void Controller::on_rest_connected() {
}

void Controller::on_rest_disconnected() {
}

// websocket api:

void Controller::on_ws_ready() {
  /*
  _gateway.websocket().send_subscribe_common();
  _gateway.rest().enqueue(
      "/products",
      [this](const std::string_view& body) {
        (*this)(json::Products::parse_message(body, _decode_buffer));
      },
      []() {
        LOG(FATAL) << "Failed to get products";
      });
      */
}

void Controller::on_ws_disconnect() {
}

void Controller::on_fix_connected() {
  auto receive_time = core::get_system_clock();
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::LOGIN_SENT,
  };
  _gateway.dispatcher().enqueue(
      order_manager_status,
      receive_time,
      receive_time,
      true);
  auto now = core::get_realtime_clock();
  auto raw_data = Random::create_raw_data(now);
  auto password = Random::create_password(raw_data, _access_secret);
  auto message = fix::Logon::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      PING_FREQUENCY.count(),
      raw_data,
      _access_key,
      password,
      CANCEL_ON_DISCONNECT);
  _gateway.fix().send(message);
}

void Controller::on_fix_disconnected() {
  auto receive_time = core::get_system_clock();
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::LOGGED_OUT,
  };
  _gateway.dispatcher().enqueue(
      order_manager_status,
      receive_time,
      receive_time,
      true);
}

void Controller::operator()(
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
            auto message = fix::OrderCancelReplaceRequest::encode(
                _encode_buffer,
                _msg_seq_num,
                core::get_realtime_clock(),
                execution_report.orig_cl_ord_id,
                execution_report.cl_ord_id,
                core::fix::Side::BUY,
                1.0,
                core::fix::OrdType::LIMIT,
                1.0,
                SYMBOL,
                execution_report.transact_time);
            _gateway.fix().send(message);
          } else {
            auto message = fix::OrderCancelRequest::encode(
                _encode_buffer,
                _msg_seq_num,
                core::get_realtime_clock(),
                execution_report.orig_cl_ord_id,
                execution_report.cl_ord_id);
            _gateway.fix().send(message);
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

void Controller::operator()(
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

void Controller::operator()(
    const fix::Logon& logon,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "logon={}, seq_num={}",
      logon,
      seq_num);
  auto receive_time = core::get_system_clock();
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::DOWNLOADING,
  };
  _gateway.dispatcher().enqueue(
      order_manager_status,
      receive_time,
      receive_time,
      true);

  auto message_1 = fix::SecurityListRequest::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      "roq-sec-001");
  _gateway.fix().send(message_1);
  auto message_2 = fix::MarketDataRequest::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      "roq-mkt-002",
      SYMBOL);
  _gateway.fix().send(message_2);

  auto message_3 = fix::RequestForPositions::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      "roq-pos-003",
      core::fix::PosReqType::POSITIONS);
  _gateway.fix().send(message_3);

  auto message_4 = fix::UserRequest::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      "roq-usr-004",
      _access_key);
  _gateway.fix().send(message_4);

  auto message_5 = fix::OrderMassStatusRequest::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      "roq-oms-005",
      core::fix::MassStatusReqType::ORDERS);
  _gateway.fix().send(message_5);

  auto message_6 = fix::NewOrderSingle::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      "roq-ord-006",
      core::fix::Side::BUY,
      2.0,
      0.5,
      SYMBOL,
      core::fix::OrdType::LIMIT,
      core::fix::TimeInForce::GTC,
      "roq;123;345");
  _gateway.fix().send(message_6);
}

void Controller::operator()(
    const fix::Logout& logout,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "logout={}, seq_num={}",
      logout,
      seq_num);
  auto receive_time = core::get_system_clock();
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::LOGGED_OUT,
  };
  _gateway.dispatcher().enqueue(
      order_manager_status,
      receive_time,
      receive_time,
      true);

  auto message = fix::Logout::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      "i'm done");
  _gateway.fix().send(message);
  // TODO(thraneh): ...now what?
}

void Controller::operator()(
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh,
    uint64_t seq_num) {
  VLOG(3) << fmt::format(
      "market_data_incremental_refresh={}, seq_num={}",
      market_data_incremental_refresh,
      seq_num);
  auto receive_time = core::get_system_clock();
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
        _gateway.dispatcher().enqueue(
            trade_summary,
            receive_time,
            receive_time,
            true);  // FIXME(thraneh): this is *not* correct !!!
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
    _gateway.dispatcher().enqueue(
        market_by_price,
        receive_time,
        receive_time,
        true);
  }
}

void Controller::operator()(
    const fix::MarketDataRequestReject& market_data_request_reject,
    uint64_t seq_num) {
  LOG(WARNING) << fmt::format(
      "market_data_request_reject={}, seq_num={}",
      market_data_request_reject,
      seq_num);
}

void Controller::operator()(
    const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh,
    uint64_t seq_num) {
  VLOG(3) << fmt::format(
      "market_data_snapshot_full_refresh={}, seq_num={}",
      market_data_snapshot_full_refresh,
      seq_num);
  auto receive_time = core::get_system_clock();
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
  _gateway.dispatcher().enqueue(
      market_by_price,
      receive_time,
      receive_time,
      true);
}

void Controller::operator()(
    const fix::OrderCancelReject& order_cancel_reject,
    uint64_t seq_num) {
  LOG(WARNING) << fmt::format(
      "order_cancel_reject={}, seq_num={}",
      order_cancel_reject,
      seq_num);
}

void Controller::operator()(
    const fix::PositionReport& position_report,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "position_report={}, seq_num={}",
      position_report,
      seq_num);
}

void Controller::operator()(
    const fix::Reject& reject,
    uint64_t seq_num) {
  LOG(WARNING) << fmt::format(
      "reject={}, seq_num={}",
      reject,
      seq_num);
}

void Controller::operator()(
    const fix::ResendRequest& resend_request,
    uint64_t seq_num) {
  LOG(WARNING) << fmt::format(
      "resend_request={}, seq_num={}",
      resend_request,
      seq_num);
  /*
  _gateway.fix().send_reject(
      msg_seq_num,
      message.header.msg_type,
      "resend_not_supported");
      */
}

void Controller::operator()(const fix::SecurityList& security_list,
    uint64_t seq_num) {
  VLOG(3) << fmt::format(
      "security_list={}, seq_num={}",
      security_list,
      seq_num);
  auto receive_time = core::get_system_clock();
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
    _gateway.dispatcher().enqueue(
        reference_data,
        receive_time,
        receive_time,
        true);
  }
}

void Controller::operator()(const fix::TestRequest& test_request,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "test_request={}, seq_num={}",
      test_request,
      seq_num);
  auto message = fix::Heartbeat::encode(
      _encode_buffer,
      _msg_seq_num,
      core::get_realtime_clock(),
      test_request.test_req_id);
  _gateway.fix().send(message);
}

void Controller::operator()(const fix::UserResponse& user_response,
    uint64_t seq_num) {
  LOG(INFO) << fmt::format(
      "user_response={}, seq_num={}",
      user_response,
      seq_num);

  auto receive_time = core::get_system_clock();
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::READY,
  };
  _gateway.dispatcher().enqueue(
      order_manager_status,
      receive_time,
      receive_time,
      true);
}

}  // namespace deribit
}  // namespace roq
