/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/market_data.h"

#include <algorithm>
#include <utility>

#include "roq/core/stack/buffer.h"

#include "roq/core/metrics/factory.h"

#include "roq/deribit/common.h"
#include "roq/deribit/flags.h"

#include "roq/core/debug.h"

using namespace roq::literals;

namespace roq {
namespace deribit {

namespace {
static const auto LOGOUT_RESPONSE = "LOGOUT"_sv;  // XXX
static const auto CONNECTION = "mkt"_sv;

static auto create_connection_name(uint32_t market_stream_id) {
  return roq::format("{}_{}"_fmt, CONNECTION, market_stream_id);
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(Flags::name(), group, function) {}
};
}  // namespace

MarketData::MarketData(
    Handler &handler,
    Security &security,
    core::io::Context &context,
    uint32_t stream_id,
    std::vector<std::string> &&symbols)
    : handler_(handler), stream_id_(stream_id), symbols_(std::move(symbols)),
      name_(create_connection_name(stream_id)), security_(security),
      connection_factory_(context, Flags::fix_uri()), connection_(*this, connection_factory_),
      encode_buffer_(Flags::encode_buffer_size()), decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"_sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"_sv),
          .market_data_incremental_refresh =
              create_metrics(name_, "market_data_incremental_refresh"_sv),
          .market_data_request_reject = create_metrics(name_, "market_data_request_reject"_sv),
          .market_data_snapshot_full_refresh =
              create_metrics(name_, "market_data_snapshot_full_refresh"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
      } {
}

bool MarketData::ready() const {
  return connection_.ready();
}

void MarketData::close() {
  // XXX send_logout?
  connection_.close();
}

void MarketData::operator()(const Event<Start> &) {
  connection_.start();
}

void MarketData::operator()(const Event<Stop> &) {
  connection_.stop();
}

void MarketData::operator()(const Event<Timer> &event) {
  if (connection_.refresh(event.value.now) == false)
    return;
  if (ready_ && next_heartbeat_ <= event.value.now) {
    assert(Flags::fix_ping_freq().count() > 0);
    next_heartbeat_ = event.value.now + Flags::fix_ping_freq();
    send_test_request(core::get_system_clock());
  }
}

void MarketData::operator()(const fix::MarketDataRequest &market_data_request) {
  VLOG(1)(R"(request(market_data_request={}))"_fmt, market_data_request);
  send(market_data_request);
}

void MarketData::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.market_data_incremental_refresh, metrics::PROFILE)
      .write(profile_.market_data_request_reject, metrics::PROFILE)
      .write(profile_.market_data_snapshot_full_refresh, metrics::PROFILE)
      .write(latency_.ping, metrics::LATENCY);
}

void MarketData::subscribe() {
  LOG(INFO)("Subscribe market data"_sv);
  assert(!symbols_.empty());
  fix::MDReq md_entry_types[] = {
      {.md_entry_type = core::fix::MDEntryType::BID},
      {.md_entry_type = core::fix::MDEntryType::OFFER},
      {.md_entry_type = core::fix::MDEntryType::TRADE},
  };
  std::vector<fix::InstrmtMDReq> related_sym(Flags::fix_market_data_request_max_size());
  for (size_t i = {};; ++i) {
    auto offset = i * Flags::fix_market_data_request_max_size();
    if (symbols_.size() < offset)
      break;
    auto count =
        std::min<size_t>(symbols_.size() - offset, Flags::fix_market_data_request_max_size());
    if (count) {
      for (size_t j = {}; j < count; ++j)
        related_sym[j].symbol = symbols_[offset + j];
      auto request_id = "test"_sv;  // XXX HANS ???
      uint32_t market_depth = 20u;  // XXX HANS should be flag
      auto md_update_type = market_depth ? core::fix::MDUpdateType::INCREMENTAL_REFRESH
                                         : core::fix::MDUpdateType::FULL_REFRESH;
      fix::MarketDataRequest market_data_request{
          .md_req_id = request_id,
          .subscription_request_type = core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
          .market_depth = market_depth,
          .md_update_type = md_update_type,
          .deribit_trade_amount = {},     // none
          .deribit_since_timestamp = {},  // none
          .no_md_entry_types = {md_entry_types, std::size(md_entry_types)},
          .no_related_sym = {related_sym.data(), count},
      };
      (*this)(market_data_request);
    }
  }
}

template <typename T>
void MarketData::send(const T &event) {
  send(event, core::get_realtime_clock());
}

template <typename T>
void MarketData::send(const T &event, std::chrono::nanoseconds sending_time) {
  core::fix::Writer writer(
      encode_buffer_,
      FIX_VERSION,
      T::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      outbound_.msg_seq_num,
      sending_time);
  auto message = event.encode(writer);
  // message.print();  // DEBUG
  connection_.send(message);
}

void MarketData::send_logon() {
  auto ping_freq = std::chrono::duration_cast<std::chrono::seconds>(Flags::fix_ping_freq());
  auto sending_time = core::get_realtime_clock();
  auto raw_data = security_.create_raw_data(
      std::chrono::duration_cast<std::chrono::milliseconds>(sending_time));
  auto password = security_.create_password(raw_data);
  fix::Logon logon{
      .heart_bt_int = static_cast<uint16_t>(ping_freq.count()),
      .raw_data_length = static_cast<uint32_t>(raw_data.length()),
      .raw_data = raw_data,
      .username = security_.get_access_key(),
      .password = password,
      .use_wordsafe_tags = false,
      .cancel_on_disconnect = Flags::fix_cancel_on_disconnect(),
      .deribit_app_id = {},
      .deribit_app_sig = {},
      .deribit_sequential = false,
      .unsubscribe_execution_reports = true,
  };
  send(logon);
}

void MarketData::send_logout(const std::string_view &text) {
  fix::Logout logout{
      .text = text,
  };
  send(logout);
}

void MarketData::send_heartbeat(const std::string_view &test_req_id) {
  fix::Heartbeat heartbeat{
      .test_req_id = test_req_id,
  };
  send(heartbeat);
}

void MarketData::send_test_request(std::chrono::nanoseconds now) {
  // request_id is current time
  stack_buffer_.clear();
  core::charconv::to_string(std::back_inserter(stack_buffer_), now.count());
  auto request_id = std::string_view(stack_buffer_.data(), stack_buffer_.size());
  fix::TestRequest test_request{
      .test_req_id = request_id,
  };
  send(test_request);
}

void MarketData::operator()(const core::net::Manager::Connected &) {
  send_logon();
}

void MarketData::operator()(const core::net::Manager::Disconnected &) {
  ++counter_.disconnect;
  outbound_ = {};
  inbound_ = {};
  ready_ = false;
  next_heartbeat_ = {};
  handler_(*this);
}

void MarketData::operator()(const core::net::Manager::Read &read) {
  auto length = read.buffer.length();
  if (length == 0)
    return;
  auto buffer = read.buffer.pullup(length);
  decltype(length) total = 0;
  for (;;) {
    // core::print_memory(buffer, length);  // DEBUG
    auto bytes = core::fix::Reader<FIX_VERSION>::dispatch(
        [&](const core::fix::message_t &message) {
          try {
            check(message.header);
            parse(message);
          } catch (std::exception &) {
            core::print_memory(buffer, length);
            core::print_string_with_escapes(buffer, length);
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
    if (Flags::fix_debug())
      core::print_string_with_escapes(buffer, bytes);  // DEBUG
  }
  if (total)
    read.buffer.drain(total);
}

void MarketData::check(const core::fix::header_t &header) {
  auto current = header.msg_seq_num;
  auto expected = inbound_.msg_seq_num + 1;
  if (ROQ_UNLIKELY(current != expected)) {
    if (expected < current) {
      LOG(WARNING)
      (R"(*** SEQUENCE GAP *** )"
       R"(current={} previous={} distance={})"_fmt,
       current,
       inbound_.msg_seq_num,
       current - inbound_.msg_seq_num);
    } else {
      LOG(WARNING)
      (R"(*** SEQUENCE REPLAY *** )"
       R"(current={} previous={} distance={})"_fmt,
       current,
       inbound_.msg_seq_num,
       inbound_.msg_seq_num - current);
    }
  }
  inbound_.msg_seq_num = current;
}

void MarketData::parse(const core::fix::message_t &message) {
  profile_.parse([&]() {
    try {
      parse_helper(message);
    } catch (std::exception &e) {
      LOG(FATAL)(R"(ERROR what="{}")"_fmt, e.what());
    }
  });
}

void MarketData::parse_helper(const core::fix::message_t &message) {
  server::TraceInfo trace_info;
  core::fix::Buffer buffer(decode_buffer_);
  switch (message.header.msg_type) {
    // session
    case core::fix::MsgType::HEARTBEAT: {
      auto heartbeat = fix::Heartbeat::create(message);
      (*this)(message.header, heartbeat, trace_info);
      break;
    }
    case core::fix::MsgType::LOGON: {
      auto logon = fix::Logon::create(message);
      (*this)(message.header, logon, trace_info);
      break;
    }
    case core::fix::MsgType::LOGOUT: {
      auto logout = fix::Logout::create(message);
      (*this)(message.header, logout, trace_info);
      break;
    }
    case core::fix::MsgType::RESEND_REQUEST: {
      auto resend_request = fix::ResendRequest::create(message);
      (*this)(message.header, resend_request, trace_info);
      break;
    }
    case core::fix::MsgType::TEST_REQUEST: {
      auto test_request = fix::TestRequest::create(message);
      (*this)(message.header, test_request, trace_info);
      break;
    }
    // ...
    case core::fix::MsgType::EXECUTION_REPORT: {
      LOG(WARNING)(R"(Unexpected msg_type={})"_fmt, message.header.msg_type);
      break;
    }
    case core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH: {
      profile_.market_data_incremental_refresh([&]() {
        auto market_data_incremental_referesh =
            fix::MarketDataIncrementalRefresh::create(message, buffer);
        (*this)(message.header, market_data_incremental_referesh, trace_info);
      });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_REQUEST_REJECT: {
      profile_.market_data_request_reject([&]() {
        auto market_data_request_reject = fix::MarketDataRequestReject::create(message);
        (*this)(message.header, market_data_request_reject, trace_info);
      });
      break;
    }
    case core::fix::MsgType::MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
      profile_.market_data_snapshot_full_refresh([&]() {
        auto market_data_snapshot_full_refresh =
            fix::MarketDataSnapshotFullRefresh::create(message, buffer);
        (*this)(message.header, market_data_snapshot_full_refresh, trace_info);
      });
      break;
    }
    case core::fix::MsgType::ORDER_CANCEL_REJECT: {
      LOG(WARNING)(R"(Unexpected msg_type={})"_fmt, message.header.msg_type);
      break;
    }
    case core::fix::MsgType::POSITION_REPORT: {
      LOG(WARNING)(R"(Unexpected msg_type={})"_fmt, message.header.msg_type);
      break;
    }
    case core::fix::MsgType::REJECT: {
      LOG(WARNING)(R"(Unexpected msg_type={})"_fmt, message.header.msg_type);
      break;
    }
    case core::fix::MsgType::SECURITY_LIST: {
      LOG(WARNING)(R"(Unexpected msg_type={})"_fmt, message.header.msg_type);
      break;
    }
    case core::fix::MsgType::SECURITY_STATUS: {
      LOG(WARNING)(R"(Unexpected msg_type={})"_fmt, message.header.msg_type);
      break;
    }
    case core::fix::MsgType::USER_RESPONSE: {
      LOG(WARNING)(R"(Unexpected msg_type={})"_fmt, message.header.msg_type);
      break;
    }
    default:
      LOG(WARNING)(R"(Unexpected msg_type={})"_fmt, message.header.msg_type);
      break;
  }
}

void MarketData::operator()(
    const core::fix::header_t &header, const fix::Heartbeat &heartbeat, const server::TraceInfo &) {
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  VLOG(3)(R"(event(header={}, heartbeat={}))"_fmt, header, heartbeat);
  if (heartbeat.test_req_id.empty() == false) {
    auto send_time = core::from_chars<uint64_t>(heartbeat.test_req_id);
    auto latency =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - decltype(now){send_time}) /
        2;  // 1-way
    server::TraceInfo trace_info;
    ExternalLatency external_latency{
        .stream_id = {},
        .name = CONNECTION,
        .latency = latency,
    };
    handler_(external_latency, trace_info);
    latency_.ping.update(latency);
  }
}

void MarketData::operator()(
    const core::fix::header_t &header, const fix::Logon &logon, const server::TraceInfo &) {
  VLOG(1)(R"(event(header={}, logon={}))"_fmt, header, logon);
  LOG(INFO)("Ready"_sv);
  assert(ready_ == false);
  ready_ = true;
  handler_(*this);
  //
  subscribe();
}

void MarketData::operator()(
    const core::fix::header_t &header, const fix::Logout &logout, const server::TraceInfo &) {
  LOG(WARNING)(R"(event(header={}, logout={}))"_fmt, header, logout);
  ready_ = false;
  // note! mandated, must send a logout response
  send_logout(LOGOUT_RESPONSE);
  LOG(INFO)("closing connection"_sv);
  connection_.close();
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::ResendRequest &resend_request,
    const server::TraceInfo &) {
  LOG(WARNING)
  (R"(event(header={}, resend_request={}))"_fmt, header, resend_request);
  LOG(INFO)("closing connection"_sv);
  connection_.close();
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::TestRequest &test_request,
    const server::TraceInfo &) {
  VLOG(1)(R"(event(header={}, test_request={}))"_fmt, header, test_request);
  send_heartbeat(test_request.test_req_id);
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::MarketDataIncrementalRefresh &market_data_incremental_refresh,
    const server::TraceInfo &trace_info) {
  VLOG(3)
  (R"(event(header={}, market_data_incremental_refresh={}))"_fmt,
   header,
   market_data_incremental_refresh);
  handler_(market_data_incremental_refresh, trace_info);
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::MarketDataRequestReject &market_data_request_reject,
    const server::TraceInfo &trace_info) {
  LOG(WARNING)
  (R"(event(header={}, market_data_request_reject={}))"_fmt, header, market_data_request_reject);
  handler_(market_data_request_reject, trace_info);
}

void MarketData::operator()(
    const core::fix::header_t &header,
    const fix::MarketDataSnapshotFullRefresh &market_data_snapshot_full_refresh,
    const server::TraceInfo &trace_info) {
  VLOG(3)
  (R"(event(header={}, market_data_snapshot_full_refresh={}))"_fmt,
   header,
   market_data_snapshot_full_refresh);
  handler_(market_data_snapshot_full_refresh, trace_info);
}

}  // namespace deribit
}  // namespace roq
