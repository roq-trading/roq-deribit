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

#include "roq/core/fix/reader.h"
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
      send_ping();
      break;
    default:
      break;
  }
  */
}

// fix:

void FIX::process_data() {
  auto length = _buffer.length();
  if (length == 0)
    return;
  auto buffer = _buffer.pullup(length);
  auto bytes = core::fix::Reader::dispatch(
      [&](const core::fix::message_t& message) {
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
                send_market_data_request();
              },
              [](const fix::Logout& logout) {
                LOG(INFO) << fmt::format("logout={}", logout);
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
              [](const fix::ResendRequest& resend_request) {
                LOG(INFO) << fmt::format("resend_request={}", resend_request);
              },
              [](const fix::SecurityList& security_list) {
                LOG(INFO) << fmt::format("security_list={}", security_list);
              },
              [](const fix::TestRequest& test_request) {
                LOG(INFO) << fmt::format("test_request={}", test_request);
              },
            },
            message,
            _decode_buffer);
      },
      buffer,
      length);
  if (bytes > 0) {
    // core::print_memory(buffer, bytes);
    // core::print_memory_as_cpp_array(buffer, bytes);
    _buffer.drain(bytes);
  }
}

void FIX::send_logon() {
  auto now = core::get_realtime_clock();
  auto raw_data = Random::create_raw_data(now);
  auto password = Random::create_password(raw_data, _access_secret);
  char buffer[4096];
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      core::fix::MsgType::LOGON,
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

void FIX::send_security_list_request() {
  char buffer[4096];
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      core::fix::MsgType::SECURITY_LIST_REQUEST,
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

void FIX::send_market_data_request() {
  char buffer[4096];
  auto message = core::fix::Writer(
      buffer,
      std::size(buffer),
      core::fix::MsgType::MARKET_DATA_REQUEST,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num)
    .write(core::fix::Field::SYMBOL, "BTC-27SEP19")
    .write(core::fix::Field::MD_REQ_ID, "123")  // TODO(thraneh): do we need a request id?
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

}  // namespace deribit
}  // namespace roq
