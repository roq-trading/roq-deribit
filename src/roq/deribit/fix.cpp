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
#include "roq/core/fix/new_order_single.h"
#include "roq/core/fix/market_data_request.h"
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
static std::random_device RANDOM_DEVICE;
static std::uniform_int_distribution<uint32_t> DISTRIBUTION;
constexpr const char *SENDER_COMP_ID = "ROQ_TRADING";
constexpr const char *TARGET_COMP_ID = "DERIBITSERVER";
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
    send_logon();
  } else {
    _controller.on_ws_disconnect();
  }
}

void FIX::on_timer() {
  /*
  auto now = core::get_time();
  if (now < _next_update)
    return;
  _next_update = now + PING_FREQUENCY;
  switch (_state) {
    case State::UPGRADED:
      send_test_request("anybody in there?");
      break;
    default:
      break;
  }
  */
}

// fix:

void FIX::process_data() {
  for (;;) {
    auto length = _buffer.length();
    if (length == 0)
      return;
    auto buffer = _buffer.pullup(length);
    auto bytes = core::fix::Reader::dispatch(
        [&](const core::fix::message_t& message) {
          try {
            fix::Parser::dispatch(
                overloaded {
                  [](const fix::ExecutionReport& execution_report) {
                    LOG(INFO) << fmt::format("execution_report={}", execution_report);
                  },
                  [](const fix::Heartbeat& heartbeat) {
                    LOG(INFO) << fmt::format("heartbeat={}", heartbeat);
                  },
                  [&](const fix::Logon& logon) {
                    LOG(INFO) << fmt::format("logon={}", logon);
                    send_security_list_request();
                    send_market_data_request("123", "BTC-27SEP19");
                    send_request_for_positions("123", core::fix::PosReqType::POSITIONS);
                    send_user_request("123");
                    // send_new_order_single();
                  },
                  [](const fix::Logout& logout) {
                    LOG(INFO) << fmt::format("logout={}", logout);
                    // TODO(thraneh): deal with this - how?
                  },
                  [](const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
                    LOG(INFO) << fmt::format("market_data_incremental_refresh={}", market_data_incremental_refresh);
                  },
                  [](const fix::MarketDataRequestReject& market_data_request_reject) {
                    LOG(INFO) << fmt::format("market_data_request_reject={}", market_data_request_reject);
                  },
                  [](const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
                    LOG(INFO) << fmt::format("market_data_snapshot_full_refresh={}", market_data_snapshot_full_refresh);
                  },
                  [](const fix::PositionReport& position_report) {
                    LOG(INFO) << fmt::format("position_report={}", position_report);
                  },
                  [](const fix::Reject& reject) {
                    LOG(INFO) << fmt::format("reject={}", reject);
                  },
                  [](const fix::ResendRequest& resend_request) {
                    LOG(INFO) << fmt::format("resend_request={}", resend_request);
                    // TODO(thraneh): send_reject
                  },
                  [](const fix::SecurityList& security_list) {
                    LOG(INFO) << fmt::format("security_list={}", security_list);
                  },
                  [&](const fix::TestRequest& test_request) {
                    LOG(INFO) << fmt::format("test_request={}", test_request);
                    send_heartbeat(test_request.test_req_id);
                  },
                  [](const fix::UserResponse& user_response) {
                    LOG(INFO) << fmt::format("user_response={}", user_response);
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

void FIX::send_logon() {
  auto now = core::get_realtime_clock();
  auto raw_data = Random::create_raw_data(now);
  auto password = Random::create_password(raw_data, _access_secret);
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::Logon>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      core::fix::Logon::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::HEART_BT_INT, uint16_t{10})
    .write(core::fix::Field::RAW_DATA, raw_data)
    .write(core::fix::Field::USERNAME, _access_key)
    .write(core::fix::Field::PASSWORD, password)
    .write(static_cast<uint32_t>(fix::Deribit::CANCEL_ON_DISCONNECT), true)
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
      core::fix::TestRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::TEST_REQ_ID, test_req_id)
    .finish();
  // core::print_memory(message);  // DEBUG
  _buffer_event.write(message);
}

void FIX::send_security_list_request() {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::SecurityListRequest>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      core::fix::SecurityListRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::SECURITY_REQ_ID, "123")  // TODO(thraneh): do we need a request id?
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

void FIX::send_new_order_single() {
  char buffer[4096];
  // auto message = core::fix::Writer<core::fix::RequestForPositions>(
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      core::fix::NewOrderSingle::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::CL_ORD_ID, "123")
    .write(core::fix::Field::SIDE, core::fix::Side::BUY)
    .write(core::fix::Field::ORDER_QTY, 1.0)
    .write(core::fix::Field::PRICE, 1.0e-8)
    .write(core::fix::Field::SYMBOL, "BTC-27SEP19")
    .finish();
  // core::print_memory(message);
  _buffer_event.write(message);
}

}  // namespace deribit
}  // namespace roq
