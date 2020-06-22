/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix.h"

#include "roq/core/stack/buffer.h"

#include "roq/deribit/common.h"
#include "roq/deribit/options.h"

#include "roq/core/debug.h"

namespace roq {
namespace deribit {

constexpr std::string_view LOGOUT_RESPONSE("LOGOUT");  // XXX

constexpr std::string_view CONNECTION("fix");

static auto create_counter(
    const std::string_view& function) {
  return core::metrics::Counter(
      FLAGS_name,
      CONNECTION,
      function);
}

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
    Handler& handler,
    const Config& config,
    Random& random,
    core::event::Base& base,
    core::event::DNSBase& dns_base)
    : _handler(handler),
      _access_key(config.get_access_key()),
      _random(random),
      _connection_factory(
          base,
          dns_base,
          FLAGS_fix_uri),
      _connection(
          *this,
          _connection_factory),
      _encode_buffer(FLAGS_encode_buffer_size),
      _decode_buffer(FLAGS_decode_buffer_size),
      _counter {
        .disconnect = create_counter("disconnect"),
      },
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
        .security_status = create_profile("security_status"),
        .user_response = create_profile("user_response"),
      },
      _latency {
        .ping = create_latency("ping"),
      } {
}

bool FIX::ready() const {
  return _connection.ready();
}

void FIX::close() {
  // XXX send_logout?
  _connection.close();
}

void FIX::operator()(const server::StartEvent&) {
  _connection.start();
}

void FIX::operator()(const server::StopEvent&) {
  _connection.stop();
}

void FIX::operator()(const server::TimerEvent& event) {
  if (_connection.refresh(event.now) == false)
    return;
  if (_ready && _next_heartbeat <= event.now) {
    assert(FLAGS_fix_ping_freq_secs > 0);
    _next_heartbeat = event.now +
      std::chrono::seconds{FLAGS_fix_ping_freq_secs};
    send_test_request(core::get_system_clock());
  }
}

void FIX::operator()(
    const fix::SecurityListRequest& security_list_request) {
  VLOG(1)(
      FMT_STRING(R"(request(security_list_request={}))"),
      security_list_request);
  send(security_list_request);
}

void FIX::operator()(
    const fix::SecurityStatusRequest& security_status_request) {
  VLOG(1)(
      FMT_STRING(R"(request(security_status_request={}))"),
      security_status_request);
  send(security_status_request);
}

void FIX::operator()(
    const fix::MarketDataRequest& market_data_request) {
  VLOG(1)(
      FMT_STRING(R"(request(market_data_request={}))"),
      market_data_request);
  send(market_data_request);
}

void FIX::operator()(
    const fix::UserRequest& user_request) {
  VLOG(1)(
      FMT_STRING(R"(request(user_request={}))"),
      user_request);
  send(user_request);
}

void FIX::operator()(
    const fix::RequestForPositions& request_for_position) {
  VLOG(1)(
      FMT_STRING(R"(request(request_for_position={}))"),
      request_for_position);
  send(request_for_position);
}

void FIX::operator()(
    const fix::OrderMassStatusRequest& order_mass_status_request) {
  VLOG(1)(
      FMT_STRING(R"(request(order_mass_status_request={}))"),
      order_mass_status_request);
  send(order_mass_status_request);
}

void FIX::operator()(
    const fix::NewOrderSingle& new_order_single) {
  VLOG(1)(
      FMT_STRING(R"(request(new_order_single={}))"),
      new_order_single);
  send(new_order_single);
}

void FIX::operator()(
    const fix::OrderCancelReplaceRequest& order_cancel_replace_request) {
  VLOG(1)(
      FMT_STRING(R"(request(order_cancel_replace_request={}))"),
      order_cancel_replace_request);
  send(order_cancel_replace_request);
}

void FIX::operator()(
    const fix::OrderCancelRequest& order_cancel_request) {
  VLOG(1)(
      FMT_STRING(R"(request(order_cancel_request={}))"),
      order_cancel_request);
  send(order_cancel_request);
}

void FIX::operator()(metrics::Writer& writer) {
  writer
    // counter
    .write(_counter.disconnect, metrics::COUNTER)
    // profile
    .write(_profile.parse, metrics::PROFILE)
    .write(_profile.execution_report, metrics::PROFILE)
    .write(_profile.market_data_incremental_refresh, metrics::PROFILE)
    .write(_profile.market_data_request_reject, metrics::PROFILE)
    .write(_profile.market_data_snapshot_full_refresh, metrics::PROFILE)
    .write(_profile.order_cancel_reject, metrics::PROFILE)
    .write(_profile.position_report, metrics::PROFILE)
    .write(_profile.reject, metrics::PROFILE)
    .write(_profile.security_list, metrics::PROFILE)
    .write(_profile.security_status, metrics::PROFILE)
    .write(_profile.user_response, metrics::PROFILE)
    // latency
    .write(_latency.ping, metrics::LATENCY);
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
      FIX_VERSION,
      T::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      _outbound.msg_seq_num,
      sending_time);
  auto message = event.encode(writer);
  // message.print();  // DEBUG
  _connection.send(message);
}

void FIX::send_logon() {
  auto sending_time = core::get_realtime_clock();
  auto raw_data = _random.create_raw_data(sending_time);
  auto password = _random.create_password(raw_data);
  fix::Logon logon {
    .heart_bt_int = static_cast<uint16_t>(FLAGS_fix_ping_freq_secs),
    .raw_data_length = static_cast<uint32_t>(raw_data.length()),
    .raw_data = raw_data,
    .username = _access_key,
    .password = password,
    .use_wordsafe_tags = false,
    .cancel_on_disconnect = FLAGS_fix_cancel_on_disconnect,
    .deribit_app_id = std::string_view(),
    .deribit_app_sig = std::string_view(),
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

void FIX::send_test_request(std::chrono::nanoseconds now) {
  // request_id is current time
  _stack_buffer.clear();
  core::charconv::to_string(
      std::back_inserter(_stack_buffer),
      now.count());
  auto request_id = std::string_view(
      _stack_buffer.data(),
      _stack_buffer.size());
  fix::TestRequest test_request {
    .test_req_id = request_id,
  };
  send(test_request);
}

void FIX::operator()(const core::net::Manager::Connected&) {
  send_logon();
}

void FIX::operator()(const core::net::Manager::Disconnected&) {
  ++_counter.disconnect;
  _outbound = {};
  _inbound = {};
  _ready = false;
  _next_heartbeat = {};
  _handler(*this);
}

void FIX::operator()(const core::net::Manager::Read& read) {
  auto length = read.buffer.length();
  if (length == 0)
    return;
  auto buffer = read.buffer.pullup(length);
  decltype(length) total = 0;
  for (;;) {
    // core::print_memory(buffer, length);  // DEBUG
    auto bytes = core::fix::Reader<FIX_VERSION>::dispatch(
        [&](const core::fix::message_t& message) {
          try {
            check(message.header);
            parse(message);
          } catch (std::exception&) {
            core::print_memory(
                buffer,
                length);
            core::print_string_with_escapes(
                buffer,
                length);
            throw;
          }
        },
        buffer,
        length);
    assert(bytes <= length);
    if (bytes == 0)
      break;
    total += bytes;
    buffer += bytes;
    length -= bytes;
    if (FLAGS_fix_debug)
      core::print_string_with_escapes(buffer, bytes);  // DEBUG
  }
  if (total)
    read.buffer.drain(total);
}

void FIX::check(const core::fix::header_t& header) {
  auto current = header.msg_seq_num;
  auto expected = _inbound.msg_seq_num + 1;
  if (ROQ_PREDICT_FALSE(current != expected)) {
    if (expected < current) {
      LOG(WARNING)(
          FMT_STRING(
            R"(*** SEQUENCE GAP *** )"
            R"(current={} previous={} distance={})"),
          current,
          _inbound.msg_seq_num,
          current - _inbound.msg_seq_num);
    } else {
      LOG(WARNING)(
          FMT_STRING(
            R"(*** SEQUENCE REPLAY *** )"
            R"(current={} previous={} distance={})"),
          current,
          _inbound.msg_seq_num,
          _inbound.msg_seq_num - current);
    }
  }
  _inbound.msg_seq_num = current;
}

void FIX::parse(const core::fix::message_t& message) {
  _profile.parse(
      [&]() {
        try {
          parse_helper(message);
        } catch (std::exception& e) {
          LOG(FATAL)(
              FMT_STRING(R"(ERROR what="{}")"),
              e.what());
        }
      });
}

void FIX::parse_helper(const core::fix::message_t& message) {
  server::Trace trace;
  core::fix::Buffer buffer(_decode_buffer);
  switch (message.header.msg_type) {
    // session
    case core::fix::MsgType::HEARTBEAT: {
      auto heartbeat = fix::Heartbeat::create(message);
      (*this)(
          message.header,
          heartbeat,
          trace);
      break;
    }
    case core::fix::MsgType::LOGON: {
      auto logon = fix::Logon::create(message);
      (*this)(
          message.header,
          logon,
          trace);
      break;
    }
    case core::fix::MsgType::LOGOUT: {
      auto logout = fix::Logout::create(message);
      (*this)(
          message.header,
          logout,
          trace);
      break;
    }
    case core::fix::MsgType::RESEND_REQUEST: {
      auto resend_request = fix::ResendRequest::create(message);
      (*this)(
          message.header,
          resend_request,
          trace);
      break;
    }
    case core::fix::MsgType::TEST_REQUEST: {
      auto test_request = fix::TestRequest::create(message);
      (*this)(
          message.header,
          test_request,
          trace);
      break;
    }
    // ...
    case core::fix::MsgType::EXECUTION_REPORT: {
      _profile.execution_report(
          [&]() {
            auto execution_report =
              fix::ExecutionReport::create(
                  message,
                  buffer);
            (*this)(
                message.header,
                execution_report,
                trace);
          });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH: {
      _profile.market_data_incremental_refresh(
          [&]() {
            auto market_data_incremental_referesh =
                fix::MarketDataIncrementalRefresh::create(
                    message,
                    buffer);
            (*this)(
                message.header,
                market_data_incremental_referesh,
                trace);
          });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_REQUEST_REJECT: {
      _profile.market_data_request_reject(
          [&]() {
            auto market_data_request_reject =
              fix::MarketDataRequestReject::create(message);
            (*this)(
                message.header,
                market_data_request_reject,
                trace);
          });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
      _profile.market_data_snapshot_full_refresh(
          [&]() {
            auto market_data_snapshot_full_refresh =
              fix::MarketDataSnapshotFullRefresh::create(
                  message,
                  buffer);
            (*this)(
                message.header,
                market_data_snapshot_full_refresh,
                trace);
          });
      break;
    }
    case core::fix::MsgType::ORDER_CANCEL_REJECT: {
      _profile.order_cancel_reject(
          [&]() {
            auto order_cancel_reject =
              fix::OrderCancelReject::create(message);
            (*this)(
                message.header,
                order_cancel_reject,
                trace);
          });
      break;
    }
    case core::fix::MsgType::POSITION_REPORT: {
      _profile.position_report(
          [&]() {
            auto position_report =
              fix::PositionReport::create(
                  message,
                  buffer);
            (*this)(
                message.header,
                position_report,
                trace);
          });
      break;
    }
    case core::fix::MsgType::REJECT: {
      _profile.reject(
          [&]() {
            auto reject = fix::Reject::create(message);
            (*this)(
                message.header,
                reject,
                trace);
          });
      break;
    }
    case core::fix::MsgType::SECURITY_LIST: {
      _profile.security_list(
          [&]() {
            auto security_list =
              fix::SecurityList::create(
                  message,
                  buffer);
            (*this)(
                message.header,
                security_list,
                trace);
          });
      break;
    }
    case core::fix::MsgType::SECURITY_STATUS: {
      _profile.security_status(
          [&]() {
            auto security_status =
              fix::SecurityStatus::create(
                  message,
                  buffer);
            (*this)(
                message.header,
                security_status,
                trace);
          });
      break;
    }
    case core::fix::MsgType::USER_RESPONSE: {
      _profile.user_response(
          [&]() {
            auto user_response = fix::UserResponse::create(message);
            (*this)(
                message.header,
                user_response,
                trace);
          });
      break;
    }
    default:
      LOG(WARNING)(
          FMT_STRING(R"(Unexpected msg_type={})"),
          message.header.msg_type);
      break;
  }
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::Heartbeat& heartbeat,
    const server::Trace& trace) {
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  VLOG(3)(
      FMT_STRING(R"(event(header={}, heartbeat={}))"),
      header,
      heartbeat);
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
    const fix::Logon& logon,
    const server::Trace& trace) {
  VLOG(1)(
      FMT_STRING(R"(event(header={}, logon={}))"),
      header,
      logon);
  LOG(INFO)("Ready");
  assert(_ready == false);
  _ready = true;
  _handler(*this);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::Logout& logout,
    const server::Trace& trace) {
  LOG(WARNING)(
      FMT_STRING(R"(event(header={}, logout={}))"),
      header,
      logout);
  _ready = false;
  // note! mandated, must send a logout response
  send_logout(LOGOUT_RESPONSE);
  LOG(INFO)("closing connection");
  _connection.close();
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::ResendRequest& resend_request,
    const server::Trace& trace) {
  LOG(WARNING)(
      FMT_STRING(R"(event(header={}, resend_request={}))"),
      header,
      resend_request);
  LOG(INFO)("closing connection");
  _connection.close();
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::TestRequest& test_request,
    const server::Trace& trace) {
  VLOG(1)(
      FMT_STRING(R"(event(header={}, test_request={}))"),
      header,
      test_request);
  send_heartbeat(test_request.test_req_id);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::ExecutionReport& execution_report,
    const server::Trace& trace) {
  VLOG(3)(
      FMT_STRING(R"(event(header={}, execution_report={}))"),
      header,
      execution_report);
  _handler(
      execution_report,
      trace);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh,
    const server::Trace& trace) {
  VLOG(3)(
      FMT_STRING(R"(event(header={}, market_data_incremental_refresh={}))"),
      header,
      market_data_incremental_refresh);
  _handler(
      market_data_incremental_refresh,
      trace);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataRequestReject& market_data_request_reject,
    const server::Trace& trace) {
  LOG(WARNING)(
      FMT_STRING(R"(event(header={}, market_data_request_reject={}))"),
      header,
      market_data_request_reject);
  _handler(
      market_data_request_reject,
      trace);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh,
    const server::Trace& trace) {
  VLOG(3)(
      FMT_STRING(R"(event(header={}, market_data_snapshot_full_refresh={}))"),
      header,
      market_data_snapshot_full_refresh);
  _handler(
      market_data_snapshot_full_refresh,
      trace);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::OrderCancelReject& order_cancel_reject,
    const server::Trace& trace) {
  VLOG(3)(
      FMT_STRING(R"(event(header={}, order_cancel_reject={}))"),
      header,
      order_cancel_reject);
  _handler(
      order_cancel_reject,
      trace);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::PositionReport& position_report,
    const server::Trace& trace) {
  VLOG(3)(
      FMT_STRING(R"(event(header={}, position_report={}))"),
      header,
      position_report);
  _handler(
      position_report,
      trace);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::Reject& reject,
    const server::Trace& trace) {
  VLOG(3)(
      FMT_STRING(R"(event(header={}, reject={}))"),
      header,
      reject);
  _handler(
      reject,
      trace);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::SecurityList& security_list,
    const server::Trace& trace) {
  VLOG(2)(
      FMT_STRING(R"(event(header={}, security_list={}))"),
      header,
      security_list);
  _handler(
      security_list,
      trace);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::SecurityStatus& security_status,
    const server::Trace& trace) {
  VLOG(2)(
      FMT_STRING(R"(event(header={}, security_status={}))"),
      header,
      security_status);
  _handler(
      security_status,
      trace);
}

void FIX::operator()(
    const core::fix::header_t& header,
    const fix::UserResponse& user_response,
    const server::Trace& trace) {
  VLOG(2)(
      FMT_STRING(R"(event(header={}, user_response={}))"),
      header,
      user_response);
  _handler(
      user_response,
      trace);
}

}  // namespace deribit
}  // namespace roq
