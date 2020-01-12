/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix.h"

#include "roq/core/stack/buffer.h"

#include "roq/deribit/gateway.h"
#include "roq/deribit/options.h"
#include "roq/deribit/random.h"

#include "roq/core/debug.h"

#define PREFIX "[FIX] "

namespace roq {
namespace deribit {

constexpr std::string_view SENDER_COMP_ID("ROQ_TRADING");
constexpr std::string_view TARGET_COMP_ID("DERIBITSERVER");

constexpr std::string_view LOGOUT_RESPONSE("LOGOUT");  // XXX

constexpr std::string_view CONNECTION("fix");

static auto create_profile(
    const std::string_view& function) {
  return core::metrics::Profile(
      FLAGS_name,
      CONNECTION,
      function);
}

static auto create_latency(
    const std::string_view& function) {
  return core::metrics::Latency(
      FLAGS_name,
      CONNECTION,
      function);
}

FIX::FIX(
    Gateway& gateway,
    const Config& config,
    core::event::Base& base,
    core::event::DNSBase& dns_base)
    : _gateway(gateway),
      _access_key(config.get_access_key()),
      _access_secret(config.get_access_secret()),
      _connection_factory(
          base,
          dns_base,
          FLAGS_fix_uri),
      _connection(
          *this,
          _connection_factory),
      _encode_buffer(FLAGS_encode_buffer_size),
      _decode_buffer(FLAGS_decode_buffer_size),
      _profile {
        .parse = create_profile("parse"),
        .execution_report = create_profile("execution_report"),
        .market_data_incremental_refresh = create_profile(
            "market_data_incremental_refresh"),
        .market_data_request_reject = create_profile(
            "market_data_request_reject"),
        .market_data_snapshot_full_refresh = create_profile(
            "market_data_snapshot_full_refresh"),
        .order_cancel_reject = create_profile("order_cancel_reject"),
        .position_report = create_profile("position_report"),
        .reject = create_profile("reject"),
        .security_list = create_profile("security_list"),
        .user_response = create_profile("user_response"),
      },
      _latency {
        .ping = create_latency("ping"),
      } {
}

bool FIX::ready() const {
  return _state == State::READY;
}

std::string_view FIX::next_request_id() {
  _buffer.clear();
  // XXX core::stack::Buffer::insert ???
  // _buffer.insert(
  //     _buffer.end(),
  //     {'r', 'o', 'q', '-'});
  core::charconv::to_string(
      std::back_inserter(_buffer),
      ++_request_id);
  return std::string_view(
      _buffer.data(),
      _buffer.size());
}

void FIX::operator()(const StartEvent&) {
  _connection.start();
}

void FIX::operator()(const StopEvent&) {
  _connection.stop();
}

void FIX::operator()(const TimerEvent& event) {
  auto now = event.now;
  switch (_state) {
    case State::READY:
      if (_next_heartbeat <= now) {
        _next_heartbeat = now +
          std::chrono::seconds{FLAGS_ping_freq_secs};
        send_heartbeat(now);
      }
      break;
    default:
      _connection.refresh(now);
      break;
  }
}

void FIX::operator()(
    const fix::MarketDataRequest& market_data_request) {
  VLOG(1)(PREFIX
      "request(market_data_request={})",
      market_data_request);
  send(market_data_request);
}

void FIX::operator()(
    const fix::NewOrderSingle& new_order_single) {
  VLOG(1)(PREFIX
      "request(new_order_single={})",
      new_order_single);
  send(new_order_single);
}

void FIX::operator()(
    const fix::OrderCancelReplaceRequest& order_cancel_replace_request) {
  VLOG(1)(PREFIX
      "request(order_cancel_replace_request={})",
      order_cancel_replace_request);
  send(order_cancel_replace_request);
}

void FIX::operator()(
    const fix::OrderCancelRequest& order_cancel_request) {
  VLOG(1)(PREFIX
      "request(order_cancel_request={})",
      order_cancel_request);
  send(order_cancel_request);
}

void FIX::operator()(
    const fix::OrderMassStatusRequest& order_mass_status_request) {
  VLOG(1)(PREFIX
      "request(order_mass_status_request={})",
      order_mass_status_request);
  send(order_mass_status_request);
}

void FIX::operator()(
    const fix::RequestForPositions& request_for_position) {
  VLOG(1)(PREFIX
      "request(request_for_position={})",
      request_for_position);
  send(request_for_position);
}

void FIX::operator()(
    const fix::SecurityListRequest& security_list_request) {
  VLOG(1)(PREFIX
      "request(security_list_request={})",
      security_list_request);
  send(security_list_request);
}

void FIX::operator()(
    const fix::UserRequest& user_request) {
  VLOG(1)(PREFIX
      "request(user_request={})",
      user_request);
  send(user_request);
}

void FIX::operator()(Metrics& metrics) {
  metrics
    // profile
    .write(_profile.parse)
    .write(_profile.execution_report)
    .write(_profile.market_data_incremental_refresh)
    .write(_profile.market_data_request_reject)
    .write(_profile.market_data_snapshot_full_refresh)
    .write(_profile.order_cancel_reject)
    .write(_profile.position_report)
    .write(_profile.reject)
    .write(_profile.security_list)
    .write(_profile.user_response)
    // latency
    .write(_latency.ping);
}

template <typename T>
void FIX::send(const T& event) {
  send(event, core::get_realtime_clock());
}

template <typename T>
void FIX::send(
    const T& event,
    std::chrono::nanoseconds sending_time) {
  core::fix::Writer writer(
      _encode_buffer,
      fix::FIX_VERSION,
      T::MSG_TYPE,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _msg_seq_num,
      sending_time);
  auto message = event.encode(writer);
  // message.print();  // DEBUG
  _connection.send(message);
}

void FIX::send_logon() {
  LOG(INFO)(PREFIX
      "sending logon request (username=\"{}\")...",
      _access_key);
  auto sending_time = core::get_realtime_clock();
  auto raw_data = Random::create_raw_data(sending_time);
  auto password = Random::create_password(raw_data, _access_secret);
  fix::Logon logon {
    .heart_bt_int = static_cast<uint16_t>(FLAGS_ping_freq_secs),
    .raw_data = raw_data,
    .username = _access_key,
    .password = password,
    .deribit_cancel_on_disconnect = FLAGS_cancel_on_disconnect,
  };
  send(logon);
}

void FIX::send_logout(const std::string_view& text) {
  fix::Logout logout {
    .text = text,
  };
  send(logout);
}

void FIX::send_heartbeat(const std::string_view& test_req_id) {
  fix::Heartbeat heartbeat {
    .test_req_id = test_req_id,
  };
  send(heartbeat);
}

void FIX::send_heartbeat(std::chrono::nanoseconds now) {
  _buffer.clear();
  core::charconv::to_string(
      std::back_inserter(_buffer),
      now.count());
  fix::TestRequest test_request {
    .test_req_id = std::string_view(
        _buffer.data(),
        _buffer.size()),
  };
  send(test_request);
}

void FIX::operator()(State state) {
  auto previous = ready();
  _state = state;
  if (ready() != previous)
    _gateway(*this);
}

void FIX::operator()(const core::net::Manager::Connected&) {
  assert(_state == State::DISCONNECTED);
  send_logon();
  (*this)(State::LOGON_SENT);
}

void FIX::operator()(const core::net::Manager::Disconnected&) {
  _msg_seq_num = 0;
  _their_msg_seq_num = 0;
  (*this)(State::DISCONNECTED);
}

void FIX::operator()(const core::net::Manager::Read& read) {
  auto length = read.buffer.length();
  if (length) {
    auto buffer = read.buffer.pullup(length);
    // core::print_memory(buffer, length);  // DEBUG
    auto bytes = core::fix::Reader<fix::FIX_VERSION>::dispatch(
        [&](const core::fix::message_t& message) {
          try {
            check(message.header);
            parse(message);
          } catch (std::exception&) {
            core::print_memory(buffer, length);
            core::print_string_with_escapes(buffer, length);
            throw;
          }
        },
        buffer,
        length);
    if (bytes == 0)
      return;
    /*
    if (FLAGS_log_fix)
      core::print_string_with_escapes(buffer, bytes);  // DEBUG
    */
    read.buffer.drain(bytes);
  }
}

void FIX::check(const core::fix::header_t& header) {
  auto current = header.msg_seq_num;
  auto expected = _their_msg_seq_num + 1;
  if (unlikely(current != expected)) {
    if (expected < current) {
      LOG(WARNING)(PREFIX
          "*** SEQUENCE GAP *** current={} previous={} distance={}",
          current,
          _their_msg_seq_num,
          current - _their_msg_seq_num);
    } else {
      LOG(WARNING)(PREFIX
          "*** SEQUENCE REPLAY *** current={} previous={} distance={}",
          current,
          _their_msg_seq_num,
          _their_msg_seq_num - current);
    }
  }
  _their_msg_seq_num = current;
}

void FIX::parse(const core::fix::message_t& message) {
  _profile.parse(
      [&]() {
        try {
          parse_helper(message);
        } catch (std::exception& e) {
          LOG(FATAL)("ERROR what=\"{}\"", e.what());
        }
      });
}

void FIX::parse_helper(const core::fix::message_t& message) {
  core::fix::Buffer buffer(_decode_buffer);
  switch (message.header.msg_type) {
    // session
    case core::fix::MsgType::HEARTBEAT: {
      (*this)(
          message.header,
          fix::Heartbeat::parse(message));
      break;
    }
    case core::fix::MsgType::LOGON: {
      (*this)(
          message.header,
          fix::Logon::parse(message));
      break;
    }
    case core::fix::MsgType::LOGOUT: {
      (*this)(
          message.header,
          fix::Logout::parse(message));
      break;
    }
    case core::fix::MsgType::RESEND_REQUEST: {
      (*this)(
          message.header,
          fix::ResendRequest::parse(message));
      break;
    }
    case core::fix::MsgType::TEST_REQUEST: {
      (*this)(
          message.header,
          fix::TestRequest::parse(message));
      break;
    }
    // ...
    case core::fix::MsgType::EXECUTION_REPORT: {
      _profile.execution_report(
          [&]() {
            (*this)(
                message.header,
                fix::ExecutionReport::parse(
                  message,
                  buffer));
          });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH: {
      _profile.market_data_incremental_refresh(
          [&]() {
            (*this)(
                message.header,
                fix::MarketDataIncrementalRefresh::parse(
                  message,
                  buffer));
          });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_REQUEST_REJECT: {
      _profile.market_data_request_reject(
          [&]() {
            (*this)(
                message.header,
                fix::MarketDataRequestReject::parse(message));
          });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
      _profile.market_data_snapshot_full_refresh(
          [&]() {
            (*this)(
                message.header,
                fix::MarketDataSnapshotFullRefresh::parse(
                  message,
                  buffer));
          });
      break;
    }
    case core::fix::MsgType::ORDER_CANCEL_REJECT: {
      _profile.order_cancel_reject(
          [&]() {
            (*this)(
                message.header,
                fix::OrderCancelReject::parse(message));
          });
      break;
    }
    case core::fix::MsgType::POSITION_REPORT: {
      _profile.position_report(
          [&]() {
            (*this)(
                message.header,
                fix::PositionReport::parse(message, buffer));
          });
      break;
    }
    case core::fix::MsgType::REJECT: {
      _profile.reject(
          [&]() {
            (*this)(
                message.header,
                fix::Reject::parse(message));
          });
      break;
    }
    case core::fix::MsgType::SECURITY_LIST: {
      _profile.security_list(
          [&]() {
            (*this)(
                message.header,
                fix::SecurityList::parse(message, buffer));
          });
      break;
    }
    case core::fix::MsgType::USER_RESPONSE: {
      _profile.user_response(
          [&]() {
            (*this)(
                message.header,
                fix::UserResponse::parse(message));
          });
      break;
    }
    default:
      LOG(WARNING)(PREFIX
          "Unexpected msg_type={}",
          message.header.msg_type);
      break;
  }
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::Heartbeat& heartbeat) {
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  VLOG(3)(PREFIX
      "event(header={}, heartbeat={})",
      header,
      heartbeat);
  // assert(_gateway_status != GatewayStatus::DISCONNECTED);
  if (heartbeat.test_req_id.empty() == false) {
    auto send_time = core::from_chars<uint64_t>(heartbeat.test_req_id);
    auto latency =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          now - decltype(now){send_time}) / 2;  // 1-way
    _latency.ping.update(latency.count());
  }
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::Logon& logon) {
  VLOG(1)(PREFIX
      "event(header={}, logon={})",
      header,
      logon);
  assert(_state == State::LOGON_SENT);
  LOG(INFO)(PREFIX "logon COMPLETED");
  (*this)(State::READY);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::Logout& logout) {
  LOG(WARNING)(PREFIX
      "event(header={}, logout={})",
      header,
      logout);
  // assert(_gateway_status == GatewayStatus::READY);
  // update(GatewayStatus::LOGGED_OUT);
  // note! mandated, must send a logout response
  send_logout(LOGOUT_RESPONSE);
  LOG(INFO)(PREFIX "closing connection");
  _connection.close();
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::ResendRequest& resend_request) {
  LOG(WARNING)(PREFIX
      "event(header={}, resend_request={})",
      header,
      resend_request);
  /*
  fix::Reject reject {
    .session_reject_reason = core::fix::SessionRejectReason::OTHER,
    .ref_seq_num = header.msg_seq_num,
    .ref_tag_id = 0,
    .ref_msg_type = header.msg_type_raw,
    .text = RESEND_NOT_SUPPORTED,
  };
  send(reject);
  */
  LOG(INFO)(PREFIX "closing connection");
  _connection.close();
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::TestRequest& test_request) {
  VLOG(1)(PREFIX
      "event(header={}, test_request={})",
      header,
      test_request);
  // assert(_gateway_status != GatewayStatus::DISCONNECTED);
  send_heartbeat(test_request.test_req_id);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::ExecutionReport& execution_report) {
  VLOG(3)(PREFIX
      "event(header={}, execution_report={})",
      header,
      execution_report);
  _gateway(execution_report);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
  VLOG(3)(PREFIX
      "event(header={}, market_data_incremental_refresh={})",
      header,
      market_data_incremental_refresh);
  _gateway(market_data_incremental_refresh);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataRequestReject& market_data_request_reject) {
  LOG(WARNING)(PREFIX
      "event(header={}, market_data_request_reject={})",
      header,
      market_data_request_reject);
  _gateway(market_data_request_reject);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
  VLOG(3)(PREFIX
      "event(header={}, market_data_snapshot_full_refresh={})",
      header,
      market_data_snapshot_full_refresh);
  _gateway(market_data_snapshot_full_refresh);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::OrderCancelReject& order_cancel_reject) {
  VLOG(3)(PREFIX
      "event(header={}, order_cancel_reject={})",
      header,
      order_cancel_reject);
  _gateway(order_cancel_reject);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::PositionReport& position_report) {
  VLOG(3)(PREFIX
      "event(header={}, position_report={})",
      header,
      position_report);
  _gateway(position_report);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::Reject& reject) {
  VLOG(3)(PREFIX
      "event(header={}, reject={})",
      header,
      reject);
  _gateway(reject);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::SecurityList& security_list) {
  VLOG(2)(PREFIX
      "event(header={}, security_list={})",
      header,
      security_list);
  _gateway(security_list);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::UserResponse& user_response) {
  VLOG(2)(PREFIX
      "event(header={}, user_response={})",
      header,
      user_response);
  _gateway(user_response);
}

}  // namespace deribit
}  // namespace roq
