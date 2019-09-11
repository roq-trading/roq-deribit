/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix.h"

#include <openssl/sha.h>

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <cinttypes>
#include <random>

#include "roq/patterns.h"

#include "roq/core/base64.h"
#include "roq/core/clock.h"
#include "roq/core/debug.h"

#include "roq/core/fix/heartbeat.h"
#include "roq/core/fix/logon.h"
#include "roq/core/fix/logout.h"
#include "roq/core/fix/new_order_single.h"
#include "roq/core/fix/market_data_request.h"
#include "roq/core/fix/order_cancel_replace_request.h"
#include "roq/core/fix/order_cancel_request.h"
#include "roq/core/fix/order_mass_status_request.h"
#include "roq/core/fix/reject.h"
#include "roq/core/fix/security_list_request.h"
#include "roq/core/fix/reader.h"
#include "roq/core/fix/request_for_positions.h"
#include "roq/core/fix/test_request.h"
#include "roq/core/fix/user_request.h"
#include "roq/core/fix/writer.h"

#include "roq/deribit/random.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/parser.h"

namespace roq {
namespace deribit {

namespace {
constexpr auto PING_FREQUENCY = std::chrono::seconds{10};
constexpr auto DECODE_BUFFER_SIZE = size_t{1048576};  // FIXME(thraneh): flag
constexpr auto FIX_VERSION = core::fix::Version::FIX_44;
constexpr const char *SENDER_COMP_ID = "ROQ_TRADING";
constexpr const char *TARGET_COMP_ID = "DERIBITSERVER";
constexpr const char *SYMBOL = "BTC-27SEP19";
constexpr bool CANCEL_ON_DISCONNECT = true;
}  // namespace

FIX::FIX(
    Controller& controller,
    core::ssl::Context& ssl_context,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    const core::URI& uri,
    const std::string_view& access_key,
    const std::string_view& access_secret)
    : _controller(controller),
      _ssl_connection(ssl_context),
      _dns_base(dns_base),
      _uri(uri),
      _access_key(access_key),
      _access_secret(access_secret),
      _timer(base, EV_PERSIST, [this]() { on_timer(); }),
      _buffer_event(base),  //, _ssl_connection),
      _decode_buffer(DECODE_BUFFER_SIZE) {
  LOG_IF(FATAL, _uri.scheme.compare("tcp") != 0) <<
    "Expected URI scheme to be \"tcp\" (got \"" << _uri.scheme << "\")";
  int value = 1;
  setsockopt(_buffer_event.getfd(), IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
  _timer.add(std::chrono::seconds{1});
  _buffer_event.setcb(
      [this]() { on_read(); },
      [this](int events) { on_error(events); });
  _buffer_event.enable(EV_READ);
}

void FIX::start() {
  VLOG(1) << "connect("
    "host=\"" << _uri.host << "\", "
    "port=" << _uri.get_port_with_default() <<
    ")";
  _buffer_event.connect(
      _dns_base,
      AF_INET,
      _uri.host,
      _uri.get_port_with_default());
}

void FIX::send(const std::string_view& message) {
  VLOG(4) << "send(length=" << message.length() << ")";
  _buffer_event.write(message.data(), message.length());
  _buffer_event.flush(EV_WRITE, BEV_FLUSH);
}

// bufferevent:

void FIX::on_read() {
  _buffer_event.read(_buffer);
  process_data();
}

void FIX::on_error(int events) {
  if (events & BEV_EVENT_CONNECTED) {
    LOG(INFO) << "CONNECTED";
    send_logon(
        PING_FREQUENCY.count(),
        CANCEL_ON_DISCONNECT);
  } else {
    _controller.on_ws_disconnect();
  }
}

void FIX::on_timer() {
  auto now = core::get_time();
  if (now < _next_update)
    return;
  _next_update = now + PING_FREQUENCY;
  LOG(INFO) << "*** PING ***";
  auto test_req_id = fmt::format(
      "{}",
      core::get_system_clock().count());
  LOG(INFO) << "test_req_id=" << test_req_id;
  send_test_request(test_req_id);
}

// fix:

void FIX::process_data() {
  for (;;) {
    auto length = _buffer.length();
    if (length == 0)
      return;
    auto buffer = _buffer.pullup(length);
    auto bytes = core::fix::Reader<FIX_VERSION>::dispatch(
        [&](const core::fix::message_t& message) {
          try {
            fix::Parser::dispatch(
                overloaded {
                  [&](const fix::ExecutionReport& execution_report) {
                    LOG(INFO) << fmt::format(
                        "execution_report={}, header={}",
                        execution_report,
                        message.header);
                    switch (execution_report.exec_type) {
                      case core::fix::ExecType::ORDER_STATUS:
                        switch (execution_report.ord_status) {
                          case core::fix::OrdStatus::NEW:
                            if (execution_report.order_qty > 1.0) {
                              send_order_cancel_replace_request(
                                  execution_report.orig_cl_ord_id,
                                  execution_report.cl_ord_id,
                                  core::fix::Side::BUY,
                                  1.0,
                                  core::fix::OrdType::LIMIT,
                                  1.0,
                                  SYMBOL);
                            } else {
                              send_order_cancel_request(
                                  execution_report.orig_cl_ord_id,
                                  execution_report.cl_ord_id);
                            }
                            break;
                          default:
                            break;
                        }
                        break;
                      default:
                        break;
                    }
                  },
                  [&](const fix::Heartbeat& heartbeat) {
                    if (!heartbeat.test_req_id.empty()) {
                      auto tmp = core::charconv::from_string<uint64_t>(
                          heartbeat.test_req_id);
                      std::chrono::nanoseconds send_time{tmp};
                      auto latency = std::chrono::duration_cast<
                        std::chrono::microseconds>(core::get_system_clock() - send_time);
                      LOG(INFO) << fmt::format("*** LATENCY={} ***", latency);
                    }
                    LOG(INFO) << fmt::format(
                        "heartbeat={}, header={}",
                        heartbeat,
                        message.header);
                  },
                  [&](const fix::Logon& logon) {
                    LOG(INFO) << fmt::format(
                        "logon={}, header={}",
                        logon,
                        message.header);
                    send_security_list_request("roq-sec-001");
                    send_market_data_request("roq-mkt-002", SYMBOL);
                    send_request_for_positions("roq-pos-003", core::fix::PosReqType::POSITIONS);
                    send_user_request("roq-usr-004");
                    send_order_mass_status_request(
                        "roq-oms-005",
                        core::fix::MassStatusReqType::ORDERS);
                    send_new_order_single(
                        "roq-ord-006",
                        core::fix::Side::BUY,
                        2.0,
                        0.5,
                        SYMBOL,
                        core::fix::OrdType::LIMIT,
                        core::fix::TimeInForce::GTC,
                        "roq;123;345");
                  },
                  [&](const fix::Logout& logout) {
                    LOG(INFO) << fmt::format(
                        "logout={}, header={}",
                        logout,
                        message.header);
                    // TODO(thraneh): deal with this - how?
                  },
                  [&](const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
                    /*
                    bool found = false;
                    for (size_t i = 0; i < market_data_incremental_refresh.md_inc_grp.length; ++i) {
                      auto& item = market_data_incremental_refresh.md_inc_grp.items[i];
                      switch (item.md_entry_type) {
                        case core::fix::MDEntryType::BID:
                        case core::fix::MDEntryType::OFFER:
                          break;
                        case core::fix::MDEntryType::TRADE:
                          // md_entry_date=1568010009502000000ns,
                          // md_entry_px=10398,
                          // md_entry_size=165,
                          // md_entry_type=TRADE,
                          // md_update_action=NEW,
                          // order_id="0",
                          // secondary_order_id="0",
                          // text="2971561",
                          // index_price=10296.8,
                          // ord_status=PARTIALLY_FILLED,
                          // side=SELL,
                          // deribit_label="",
                          // deribit_liquidation="",
                          // deribit_trade_id=18490039
                          found = true;
                          break;
                        default:
                          found = true;
                      }
                    }
                    if (!found)
                      return;
                    */
                    LOG(INFO) << fmt::format(
                        "market_data_incremental_refresh={}, header={}",
                        market_data_incremental_refresh,
                        message.header);
                  },
                  [&](const fix::MarketDataRequestReject& market_data_request_reject) {
                    LOG(INFO) << fmt::format(
                        "market_data_request_reject={}, header={}",
                        market_data_request_reject,
                        message.header);
                  },
                  [&](const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
                    LOG(INFO) << fmt::format(
                        "market_data_snapshot_full_refresh={}, header={}",
                        market_data_snapshot_full_refresh,
                        message.header);
                  },
                  [&](const fix::OrderCancelReject& order_cancel_reject) {
                    LOG(INFO) << fmt::format(
                        "order_cancel_reject={}, header={}",
                        order_cancel_reject,
                        message.header);
                  },
                  [&](const fix::PositionReport& position_report) {
                    LOG(INFO) << fmt::format(
                        "position_report={}, header={}",
                        position_report,
                        message.header);
                  },
                  [&](const fix::Reject& reject) {
                    LOG(INFO) << fmt::format(
                        "reject={}, header={}",
                        reject,
                        message.header);
                  },
                  [&](const fix::ResendRequest& resend_request) {
                    LOG(INFO) << fmt::format(
                        "resend_request={}, header={}",
                        resend_request,
                        message.header);
                    send_reject(
                        message.header.msg_seq_num,
                        message.header.msg_type,
                        "resend_not_supported");
                  },
                  [&](const fix::SecurityList& security_list) {
                    LOG(INFO) << fmt::format(
                        "security_list={}, header={}",
                        security_list,
                        message.header);
                  },
                  [&](const fix::TestRequest& test_request) {
                    LOG(INFO) << fmt::format(
                        "test_request={}, header={}",
                        test_request,
                        message.header);
                    send_heartbeat(test_request.test_req_id);
                  },
                  [&](const fix::UserResponse& user_response) {
                    LOG(INFO) << fmt::format(
                        "user_response={}, header={}",
                        user_response,
                        message.header);
                  },
                },
                message,
                _decode_buffer);
          } catch (std::exception& e) {
            fprintf(stderr, "*** ERROR *** %s\n", e.what());
            core::print_memory(buffer, length);
            core::print_string_with_escapes(buffer, length);
            throw;
          }
        },
        buffer,
        length);
    if (bytes == 0)
      return;
    // core::print_string_with_escapes(buffer, bytes);
    _buffer.drain(bytes);
  }
}

void FIX::send_reject(
    uint64_t ref_seq_num,
    const std::string_view& ref_msg_type,
    const std::string_view& text) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::Reject>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::Reject::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::REF_SEQ_NUM, ref_seq_num)
    .write(core::fix::Field::REF_MSG_TYPE, ref_msg_type)
    .write(core::fix::Field::TEXT, text)
    .finish();
  // core::print_memory(message);  // DEBUG
  _buffer_event.write(message);
}

void FIX::send_reject(
    uint64_t ref_seq_num,
    const core::fix::MsgType& msg_type,
    const std::string_view& text) {
  send_reject(ref_seq_num, CodeNameMsgType(msg_type), text);
}

void FIX::send_logon(uint16_t heart_bt_int, bool cancel_on_disconnect) {
  auto now = core::get_realtime_clock();
  auto raw_data = Random::create_raw_data(now);
  auto password = Random::create_password(raw_data, _access_secret);
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::Logon>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::Logon::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::HEART_BT_INT, heart_bt_int)
    .write(core::fix::Field::RAW_DATA, raw_data)
    .write(core::fix::Field::USERNAME, _access_key)
    .write(core::fix::Field::PASSWORD, password)
    .write(
        static_cast<uint32_t>(fix::Deribit::CANCEL_ON_DISCONNECT),
        cancel_on_disconnect)
    .finish();
  // core::print_memory(message);  // DEBUG
  _buffer_event.write(message);
}

void FIX::send_logout(const std::string_view& text) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::Logout>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::Logout::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::TEXT, text)
    .finish();
  // core::print_memory(message);  // DEBUG
  _buffer_event.write(message);
}

void FIX::send_heartbeat(const std::string_view& test_req_id) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::Heartbeat>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::Heartbeat::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::TEST_REQ_ID, test_req_id)
    .finish();
  // core::print_memory(message);  // DEBUG
  _buffer_event.write(message);
}

void FIX::send_test_request(const std::string_view& test_req_id) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::TestRequest>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::TestRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::TEST_REQ_ID, test_req_id)
    .finish();
  // core::print_memory(message);  // DEBUG
  _buffer_event.write(message);
}

void FIX::send_security_list_request(
    const std::string_view& security_req_id) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::SecurityListRequest>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::SecurityListRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::SECURITY_REQ_ID, security_req_id)
    .write(
        core::fix::Field::SECURITY_LIST_REQUEST_TYPE,
        core::fix::SecurityListRequestType::ALL_SECURITIES)
    .finish();
  // core::print_memory(message);
  _buffer_event.write(message);
}

void FIX::send_market_data_request(
    const std::string_view& md_req_id,
    const std::string_view& symbol) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::MarketDataRequest>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::MarketDataRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::SYMBOL, symbol)
    .write(core::fix::Field::MD_REQ_ID, md_req_id)
    .write(
        core::fix::Field::SUBSCRIPTION_REQUEST_TYPE,
        core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES)
    .write(core::fix::Field::MARKET_DEPTH, uint8_t{20})
    .write(core::fix::Field::MD_UPDATE_TYPE, core::fix::MDUpdateType::INCREMENTAL_REFRESH)
    .write(static_cast<uint32_t>(fix::Deribit::TRADE_AMOUNT), uint32_t{0})
    .write(static_cast<uint32_t>(fix::Deribit::SINCE_TIMESTAMP), uint32_t{0})
    .write(core::fix::Field::NO_MD_ENTRY_TYPES, uint8_t{3})
    .write(core::fix::Field::MD_ENTRY_TYPE, core::fix::MDEntryType::BID)
    .write(core::fix::Field::MD_ENTRY_TYPE, core::fix::MDEntryType::OFFER)
    .write(core::fix::Field::MD_ENTRY_TYPE, core::fix::MDEntryType::TRADE)
    .finish();
  // core::print_memory(message);
  _buffer_event.write(message);
}

void FIX::send_user_request(const std::string_view& user_request_id) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::UserRequest>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::UserRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::USER_REQUEST_ID, user_request_id)
    .write(
        core::fix::Field::USER_REQUEST_TYPE,
        core::fix::UserRequestType::REQUEST_INDIVIDUAL_USER_STATUS)
    .write(core::fix::Field::USERNAME, _access_key)
    .write(
        core::fix::Field::SECURITY_LIST_REQUEST_TYPE,
        core::fix::SecurityListRequestType::ALL_SECURITIES)
    .finish();
  // core::print_memory(message);
  _buffer_event.write(message);
}

void FIX::send_request_for_positions(
    const std::string_view& pos_req_id,
    const core::fix::PosReqType& pos_req_type) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::RequestForPositions>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::RequestForPositions::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::POS_REQ_ID, pos_req_id)
    .write(core::fix::Field::POS_REQ_TYPE, pos_req_type)
    .finish();
  // core::print_memory(message);
  _buffer_event.write(message);
}

void FIX::send_order_mass_status_request(
    const std::string_view& mass_status_req_id,
    const core::fix::MassStatusReqType& mass_status_req_type) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::RequestForPositions>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::OrderMassStatusRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::MASS_STATUS_REQ_ID, mass_status_req_id)
    .write(core::fix::Field::MASS_STATUS_REQ_TYPE, mass_status_req_type)
    .finish();
  // core::print_memory(message);
  _buffer_event.write(message);
}

void FIX::send_new_order_single(
    const std::string_view& cl_ord_id,
    const core::fix::Side& side,
    double order_qty,
    double price,
    const std::string_view& symbol,
    const core::fix::OrdType& ord_type,
    const core::fix::TimeInForce& time_in_force,
    const std::string_view& deribit_label) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::RequestForPositions>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::NewOrderSingle::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::CL_ORD_ID, cl_ord_id)
    .write(core::fix::Field::SIDE, side)
    .write(core::fix::Field::ORDER_QTY, order_qty)
    .write(core::fix::Field::PRICE, price)
    .write(core::fix::Field::SYMBOL, symbol)
    .write(core::fix::Field::ORD_TYPE, ord_type)
    .write(core::fix::Field::TIME_IN_FORCE, time_in_force)
    .write(static_cast<uint32_t>(fix::Deribit::LABEL), deribit_label)
    .finish();
  // core::print_memory(message);
  _buffer_event.write(message);
}

void FIX::send_order_cancel_replace_request(
    const std::string_view& cl_ord_id,
    const std::string_view& orig_cl_ord_id,
    const core::fix::Side& side,
    double order_qty,
    const core::fix::OrdType& ord_type,
    double price,
    const std::string_view& symbol,
    std::chrono::nanoseconds transact_time) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::RequestForPositions>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::OrderCancelReplaceRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::CL_ORD_ID, cl_ord_id)
    .write(core::fix::Field::ORIG_CL_ORD_ID, orig_cl_ord_id)
    .write(core::fix::Field::TRANSACT_TIME, transact_time)
    .write(core::fix::Field::SIDE, side)
    .write(core::fix::Field::ORDER_QTY, order_qty)
    .write(core::fix::Field::ORD_TYPE, ord_type)
    .write(core::fix::Field::PRICE, price)
    .write(core::fix::Field::SYMBOL, symbol)
    .finish();
  // core::print_memory(message);
  _buffer_event.write(message);
}

void FIX::send_order_cancel_request(
    const std::string_view& cl_ord_id,
    const std::string_view& orig_cl_ord_id) {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::RequestForPositions>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      FIX_VERSION,
      core::fix::OrderCancelRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::CL_ORD_ID, cl_ord_id)
    .write(core::fix::Field::ORIG_CL_ORD_ID, orig_cl_ord_id)
    .finish();
  // core::print_memory(message);
  _buffer_event.write(message);
}

}  // namespace deribit
}  // namespace roq
