/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/order_entry.h"

#include <algorithm>
#include <utility>

#include "roq/utils/mask.h"
#include "roq/utils/safe_cast.h"
#include "roq/utils/update.h"

#include "roq/core/back_emplacer.h"
#include "roq/core/debug.h"

#include "roq/core/metrics/factory.h"

#include "roq/core/fix/debug.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/common.h"

#include "roq/deribit/flags/common.h"
#include "roq/deribit/flags/config.h"
#include "roq/deribit/flags/fix.h"

#include "roq/deribit/fix/utils.h"

// business (outbound)
#include "roq/deribit/fix/new_order_single.h"
#include "roq/deribit/fix/order_cancel_replace_request.h"
#include "roq/deribit/fix/order_cancel_request.h"
#include "roq/deribit/fix/order_mass_cancel_request.h"
#include "roq/deribit/fix/order_mass_status_request.h"
#include "roq/deribit/fix/request_for_positions.h"

using namespace std::literals;

namespace roq {
namespace deribit {

namespace {
const auto LOGOUT_RESPONSE = "LOGOUT"sv;  // XXX

const auto NAME = "om"sv;
const auto SUPPORTS = utils::Mask{
    SupportType::CREATE_ORDER,
    SupportType::MODIFY_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
    SupportType::ORDER,
    SupportType::TRADE,
    SupportType::POSITION,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

template <typename T>
void emplace(Fill &result, const T &value) {
  new (&result) Fill{
      .external_trade_id = value.fill_exec_id,
      .quantity = value.fill_qty,
      .price = value.fill_px,
      .liquidity = {},
  };
}
}  // namespace

OrderEntry::OrderEntry(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared)
    : handler_(handler), stream_id_(stream_id),
      name_(fmt::format("{}:{}:{}"sv, stream_id_, NAME, security.get_account())),
      connection_factory_(context, flags::FIX::fix_uri()), connection_(*this, connection_factory_),
      encode_buffer_(flags::Common::encode_buffer_size()),
      decode_buffer_(flags::Common::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
          .position_report = create_metrics(name_, "position_report"sv),
          .execution_report = create_metrics(name_, "execution_report"sv),
          .order_cancel_reject = create_metrics(name_, "order_cancel_reject"sv),
          .reject = create_metrics(name_, "reject"sv),
          .order_mass_cancel_report = create_metrics(name_, "order_mass_cancel_report"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
      },
      security_(security), shared_(shared),
      download_(flags::FIX::fix_request_timeout(), [this](auto state) { return download(state); }) {
}

void OrderEntry::operator()(const Event<Start> &) {
  connection_.start();
}

void OrderEntry::operator()(const Event<Stop> &) {
  connection_.stop();
}

void OrderEntry::operator()(const Event<Timer> &event) {
  if (!connection_.refresh(event.value.now))
    return;
  if (last_logon_or_heartbeat_.count() && flags::FIX::fix_request_timeout().count() &&
      (event.value.now - last_logon_or_heartbeat_) > flags::FIX::fix_request_timeout()) {
    log::warn("*** DETECTED TIMEOUT ***"sv);
    log::info("closing connection"sv);
    connection_.close();
  } else {
    if (ready_ && next_heartbeat_ <= event.value.now) {
      assert(flags::FIX::fix_ping_freq().count() > 0);
      next_heartbeat_ = event.value.now + flags::FIX::fix_ping_freq();
      send_test_request(core::get_system_clock());
    }
  }
}

uint16_t OrderEntry::operator()(
    const Event<CreateOrder> &event, const oms::Order &order, const std::string_view &request_id) {
  if (!ready())
    throw oms::NotReady("not ready"sv);
  auto &[message_info, create_order] = event;
  if (std::isfinite(create_order.stop_price))
    throw RuntimeError("stop_price not supported"sv);
  if (std::isfinite(create_order.max_show_quantity))
    throw RuntimeError("max_show_quantity not supported"sv);
  auto side = core::fix::map(create_order.side);
  auto exec_inst = fix::map(create_order.execution_instruction);
  auto ord_type = core::fix::map(create_order.order_type);
  auto time_in_force = core::fix::map(create_order.time_in_force);
  core::stack::Buffer<char, MAX_LENGTH_REQUEST_ID> buffer;
  fmt::format_to(
      std::back_inserter(buffer), "roq-{}-{}"sv, message_info.source, create_order.order_id);
  std::string_view deribit_label(std::data(buffer), std::size(buffer));
  fix::NewOrderSingle new_order_single{
      .cl_ord_id = request_id,
      .side = side,
      .order_qty = {create_order.quantity, order.quantity_decimals},
      .price = {create_order.price, order.price_decimals},
      .symbol = create_order.symbol,
      .exec_inst = exec_inst,
      .ord_type = ord_type,
      .time_in_force = time_in_force,
      .deribit_label = deribit_label,
      .deribit_adv_order_type = '\0',
  };
  auto msg_seq_num = send(new_order_single);
  // XXX HANS EXPERIMENTAL -- it's a leak / currently no way to clean up
  log::info(R"(DEBUG: msg_seq_num={} --> request_id="{}")"sv, msg_seq_num, request_id);
  msg_seq_num_to_request_id_.emplace(msg_seq_num, request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    const Event<ModifyOrder> &event,
    const oms::Order &order,
    const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  if (!ready())
    throw oms::NotReady("not ready"sv);
  const auto &modify_order = event.value;
  auto side = core::fix::map(order.side);
  auto ord_type = core::fix::map(order.order_type);
  fix::OrderCancelReplaceRequest order_cancel_replace_request{
      .orig_cl_ord_id = order.external_order_id,
      .cl_ord_id = request_id,
      .transact_time = utils::safe_cast(order.update_time_utc),
      .side = side,
      .order_qty = {modify_order.quantity, order.quantity_decimals},
      .ord_type = ord_type,
      .price = {modify_order.price, order.price_decimals},
      .symbol = order.symbol,
      .exec_inst = {},
  };
  auto msg_seq_num = send(order_cancel_replace_request);
  // XXX HANS EXPERIMENTAL -- it's a leak / currently no way to clean up
  log::info(R"(DEBUG: msg_seq_num={} --> request_id="{}")"sv, msg_seq_num, request_id);
  msg_seq_num_to_request_id_.emplace(msg_seq_num, request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    const Event<CancelOrder> &,
    const oms::Order &order,
    const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  if (!ready())
    throw oms::NotReady("not ready"sv);
  fix::OrderCancelRequest order_cancel_request{
      .cl_ord_id = request_id,
      .orig_cl_ord_id = order.external_order_id,
  };
  auto msg_seq_num = send(order_cancel_request);
  // XXX HANS EXPERIMENTAL -- it's a leak / currently no way to clean up
  log::info(R"(DEBUG: msg_seq_num={} --> request_id="{}")"sv, msg_seq_num, request_id);
  msg_seq_num_to_request_id_.emplace(msg_seq_num, request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    const Event<CancelAllOrders> &event, const std::string_view &request_id) {
  if (ready()) {
    fix::OrderMassCancelRequest order_mass_cancel_request{
        .cl_ord_id = request_id,
        .mass_cancel_request_type = core::fix::MassCancelRequestType::CANCEL_ALL_ORDERS,
        .security_type = {},
        .symbol = {},
        .currency = {},
    };
    send(order_mass_cancel_request);
  } else {
    auto &[message_info, cancel_all_orders] = event;
    log::warn(
        R"(*** NOT CONNECTED! UNABLE TO CANCEL ALL ORDERS FOR ACCOUNT="{}")"sv,
        cancel_all_orders.account);
  }
  return stream_id_;
}

void OrderEntry::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.execution_report, metrics::PROFILE)
      .write(profile_.order_cancel_reject, metrics::PROFILE)
      .write(profile_.reject, metrics::PROFILE)
      .write(profile_.order_mass_cancel_report, metrics::PROFILE)
      .write(latency_.ping, metrics::LATENCY);
}

void OrderEntry::operator()(const core::net::Manager::Connected &) {
  send_logon();
  (*this)(ConnectionStatus::LOGIN_SENT);
}

void OrderEntry::operator()(const core::net::Manager::Disconnected &) {
  ++counter_.disconnect;
  outbound_ = {};
  inbound_ = {};
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void OrderEntry::operator()(const core::net::Manager::Read &read) {
  auto buffer = read.buffer.pullup_new();
  size_t total_bytes = 0;
  while (!std::empty(buffer)) {
    auto bytes = core::fix::Reader<FIX_VERSION>::dispatch(
        [&](const core::fix::message_t &message) {
          try {
            check(message.header);
            parse(message);
          } catch (std::exception &) {
            log::warn("{}"sv, core::fix::Debug(buffer));
#if !defined(NDEBUG)
            core::print_memory(buffer);
            core::print_string_with_escapes(buffer);
#endif
            throw;
          }
        },
        buffer,
        [](auto &message) {
          log::info<0>::when(flags::FIX::fix_debug(), "{}"sv, core::fix::Debug(message));
        });
    if (bytes == 0)
      break;
    assert(bytes <= std::size(buffer));
    total_bytes += bytes;
    buffer = buffer.subspan(bytes);
  }
  read.buffer.drain(total_bytes);
}

void OrderEntry::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::FIX,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void OrderEntry::send_logon() {
  auto ping_freq = std::chrono::duration_cast<std::chrono::seconds>(flags::FIX::fix_ping_freq());
  std::chrono::milliseconds now = utils::safe_cast(core::get_realtime_clock());
  auto raw_data = security_.create_raw_data(now);
  auto password = security_.create_password(raw_data);
  log::info(
      R"(DEBUG: HASHER stream_id={}, raw_data="{}", password="{}")"sv,
      stream_id_,
      raw_data,
      password);
  fix::Logon logon{
      .heart_bt_int = static_cast<uint16_t>(ping_freq.count()),
      .raw_data_length = static_cast<uint32_t>(std::size(raw_data)),
      .raw_data = raw_data,
      .username = security_.get_access_key(),
      .password = password,
      .use_wordsafe_tags = false,
      .cancel_on_disconnect = flags::FIX::fix_cancel_on_disconnect(),
      .deribit_app_id = {},
      .deribit_app_sig = {},
      .deribit_sequential = false,
      .unsubscribe_execution_reports = false,
  };
  send(logon);
  last_logon_or_heartbeat_ = core::get_system_clock();
}

void OrderEntry::send_logout(const std::string_view &text) {
  fix::Logout logout{
      .text = text,
  };
  send(logout);
}

void OrderEntry::send_heartbeat(const std::string_view &test_req_id) {
  fix::Heartbeat heartbeat{
      .test_req_id = test_req_id,
  };
  send(heartbeat);
}

void OrderEntry::send_test_request(std::chrono::nanoseconds now) {
  // request_id is current time
  stack_buffer_.clear();
  core::charconv::to_string(std::back_inserter(stack_buffer_), now.count());
  auto request_id = std::string_view(std::data(stack_buffer_), std::size(stack_buffer_));
  fix::TestRequest test_request{
      .test_req_id = request_id,
  };
  send(test_request);
  if (!last_logon_or_heartbeat_.count())
    last_logon_or_heartbeat_ = now;
}

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    case OrderEntryState::UNDEFINED:
      assert(false);
      break;
    case OrderEntryState::POSITIONS:
      subscribe_positions();
      return 1;
    case OrderEntryState::ORDERS:
      download_orders();
      return 1;  // first ExecutionReport has the real number
    case OrderEntryState::DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return {};
  }
  assert(false);
  return {};
}

void OrderEntry::subscribe_positions() {
  auto request_id = shared_.next_request_id();
  fix::RequestForPositions request_for_positions{
      .pos_req_id = request_id,
      .pos_req_type = roq::core::fix::PosReqType::POSITIONS,
      .subscription_request_type = roq::core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
      .currency = {},
  };
  send(request_for_positions);
}

void OrderEntry::download_orders() {
  auto request_id = shared_.next_request_id();
  fix::OrderMassStatusRequest order_mass_status_request{
      .mass_status_req_id = request_id,
      .mass_status_req_type = core::fix::MassStatusReqType::ORDERS,
  };
  send(order_mass_status_request);
}

void OrderEntry::parse(const core::fix::message_t &message) {
  profile_.parse([&]() { parse_helper(message); });
}

void OrderEntry::parse_helper(const core::fix::message_t &message) {
  auto trace_info = server::create_trace_info();
  core::fix::Buffer buffer(decode_buffer_);
  switch (message.header.msg_type) {
    // session
    case core::fix::MsgType::HEARTBEAT: {
      auto heartbeat = fix::Heartbeat::create(message);
      core::fix::create_event_and_dispatch(*this, message.header, heartbeat, trace_info);
      break;
    }
    case core::fix::MsgType::LOGON: {
      auto logon = fix::Logon::create(message);
      core::fix::create_event_and_dispatch(*this, message.header, logon, trace_info);
      break;
    }
    case core::fix::MsgType::LOGOUT: {
      auto logout = fix::Logout::create(message);
      core::fix::create_event_and_dispatch(*this, message.header, logout, trace_info);
      break;
    }
    case core::fix::MsgType::RESEND_REQUEST: {
      auto resend_request = fix::ResendRequest::create(message);
      core::fix::create_event_and_dispatch(*this, message.header, resend_request, trace_info);
      break;
    }
    case core::fix::MsgType::TEST_REQUEST: {
      auto test_request = fix::TestRequest::create(message);
      core::fix::create_event_and_dispatch(*this, message.header, test_request, trace_info);
      break;
    }
    // ...
    case core::fix::MsgType::POSITION_REPORT: {
      profile_.position_report([&]() {
        auto position_report = fix::PositionReport::create(message, buffer);
        core::fix::create_event_and_dispatch(*this, message.header, position_report, trace_info);
      });
      break;
    }
    case core::fix::MsgType::EXECUTION_REPORT: {
      profile_.execution_report([&]() {
        auto execution_report = fix::ExecutionReport::create(message, buffer);
        core::fix::create_event_and_dispatch(*this, message.header, execution_report, trace_info);
      });
      break;
    }
    case core::fix::MsgType::ORDER_CANCEL_REJECT: {
      profile_.order_cancel_reject([&]() {
        auto order_cancel_reject = fix::OrderCancelReject::create(message);
        core::fix::create_event_and_dispatch(
            *this, message.header, order_cancel_reject, trace_info);
      });
      break;
    }
    case core::fix::MsgType::REJECT: {
      profile_.reject([&]() {
        auto reject = fix::Reject::create(message);
        core::fix::create_event_and_dispatch(*this, message.header, reject, trace_info);
      });
      break;
    }
    case core::fix::MsgType::ORDER_MASS_CANCEL_REPORT: {
      profile_.order_mass_cancel_report([&]() {
        auto order_mass_cancel_report = fix::OrderMassCancelReport::create(message, buffer);
        core::fix::create_event_and_dispatch(
            *this, message.header, order_mass_cancel_report, trace_info);
      });
      break;
    }
    default:
      log::warn("Unexpected msg_type={}"sv, message.header.msg_type);
      break;
  }
}

void OrderEntry::operator()(
    const core::fix::Event<fix::Heartbeat> &event, const server::TraceInfo &) {
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  auto &[header, heartbeat] = event;
  log::info<3>("event={{header={}, heartbeat={}}}"sv, header, heartbeat);
  last_logon_or_heartbeat_ = {};
  if (!std::empty(heartbeat.test_req_id)) {
    auto send_time = core::from_chars<uint64_t>(heartbeat.test_req_id);
    auto latency =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - decltype(now){send_time}) /
        2;  // 1-way
    auto trace_info = server::create_trace_info();
    ExternalLatency external_latency{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .latency = latency,
    };
    server::create_trace_and_dispatch(handler_, trace_info, external_latency);
    latency_.ping.update(latency);
  }
}

void OrderEntry::operator()(const core::fix::Event<fix::Logon> &event, const server::TraceInfo &) {
  auto &[header, logon] = event;
  log::info<2>("event={{header={}, logon={}}}"sv, header, logon);
  last_logon_or_heartbeat_ = {};
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void OrderEntry::operator()(const core::fix::Event<fix::Logout> &event, const server::TraceInfo &) {
  auto &[header, logout] = event;
  log::warn("event={{header={}, logout={}}}"sv, header, logout);
  log::info("DEBUG: HASHER stream_id={} LOGOUT"sv, stream_id_);
  ready_ = false;
  // note! mandated, must send a logout response
  send_logout(LOGOUT_RESPONSE);
  log::info("closing connection"sv);
  connection_.close();
}

void OrderEntry::operator()(
    const core::fix::Event<fix::ResendRequest> &event, const server::TraceInfo &) {
  auto &[header, resend_request] = event;
  log::warn("event={{header={}, resend_request={}}}"sv, header, resend_request);
  log::info("closing connection"sv);
  connection_.close();
}

void OrderEntry::operator()(
    const core::fix::Event<fix::TestRequest> &event, const server::TraceInfo &) {
  auto &[header, test_request] = event;
  log::info<1>("event={{header={}, test_request={}}}"sv, header, test_request);
  send_heartbeat(test_request.test_req_id);
}

void OrderEntry::operator()(
    const core::fix::Event<fix::PositionReport> &event, const server::TraceInfo &trace_info) {
  auto &[header, position_report] = event;
  log::info<2>("event={{header={}, position_report={}}}"sv, header, position_report);
  for (size_t i = 0; i < std::size(position_report.no_positions); ++i) {
    auto is_last = std::size(position_report.no_positions) == (i + 1);
    auto &position_qty = position_report.no_positions[i];
    auto long_quantity = std::max(0.0, position_qty.long_qty);
    auto short_quantity = std::max(0.0, position_qty.short_qty);
    PositionUpdate position_update{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .exchange = flags::Config::exchange(),
        .symbol = position_qty.symbol,
        .external_account = {},
        .long_quantity = long_quantity,
        .short_quantity = short_quantity,
        .long_quantity_begin = NaN,
        .short_quantity_begin = NaN,
    };
    server::create_trace_and_dispatch(handler_, trace_info, position_update, is_last);
  }
  download_.check_relaxed(OrderEntryState::POSITIONS);
}

namespace {
// execution_report:
//
// mass_status_req_type  what
// ----------------------------------------
//   ORDERS                begin download
//   *                     order update
//
// exec_type       ord_status          what
// ------------------------------------------------------------------
//   REJECTED        *                   ack failure
//   CANCELED        *                   ack success + order update
//   ORDER_STATUS    NEW                 ack success + order update (create + modify)
//   ORDER_STATUS    PARTIALLY_FILLED    order update
//   ORDER_STATUS    FILLED              order update
//   ORDER_STATUS    CANCELED            ack success

RequestType compute_request_type(core::fix::ExecType exec_type, core::fix::OrdStatus ord_status) {
  switch (exec_type) {
    case core::fix::ExecType::REJECTED:
      return {};  // any
    case core::fix::ExecType::CANCELED:
      return RequestType::CANCEL_ORDER;
    case core::fix::ExecType::ORDER_STATUS:
      switch (ord_status) {
        case core::fix::OrdStatus::NEW:
        case core::fix::OrdStatus::PARTIALLY_FILLED:
          return {};  // create or modify
        case core::fix::OrdStatus::CANCELED:
          return RequestType::CANCEL_ORDER;
          break;
        default:
          break;
      }
    default:
      break;
  }
  return {};
}

RequestStatus compute_request_status(
    core::fix::ExecType exec_type, core::fix::OrdStatus ord_status) {
  switch (exec_type) {
    case core::fix::ExecType::REJECTED:
      return RequestStatus::REJECTED;
    case core::fix::ExecType::CANCELED:
      return RequestStatus::ACCEPTED;
    case core::fix::ExecType::ORDER_STATUS:
      switch (ord_status) {
        case core::fix::OrdStatus::NEW:
        case core::fix::OrdStatus::PARTIALLY_FILLED:
        case core::fix::OrdStatus::FILLED:
        case core::fix::OrdStatus::CANCELED:
          return RequestStatus::ACCEPTED;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return {};
}

// note!
//   last traded is expected (downstream) to be the sum of all fills for this update
//   Deribit reports only the *last* fill, but includes all fills as well
//   we will therefore replace these values, when possible
std::pair<double, double> compute_last_traded(
    double last_traded_quantity, double last_traded_price, const std::span<fix::Fill> &fills) {
  if (std::empty(fills))
    return std::make_pair(last_traded_quantity, last_traded_price);
  double sum_quantity = 0.0, sum_quantity_price = 0.0;
  for (auto &fill : fills) {
    sum_quantity += fill.fill_qty;
    sum_quantity_price += fill.fill_qty * fill.fill_px;
  }
  return std::make_pair(
      sum_quantity,
      utils::compare(sum_quantity, 0.0) == 0 ? NaN : sum_quantity_price / sum_quantity);
}
}  // namespace

void OrderEntry::operator()(
    const core::fix::Event<fix::ExecutionReport> &event, const server::TraceInfo &trace_info) {
  auto &[header, execution_report] = event;
  log::info<2>("event={{header={}, execution_report={}}}"sv, header, execution_report);
  auto download = false;
  // download begin?
  switch (execution_report.mass_status_req_type) {
    case core::fix::MassStatusReqType::ORDERS:
      download = true;
      download_.update(OrderEntryState::ORDERS, execution_report.tot_num_reports);
      return;
    default:
      break;
  }
  // liquidity indicator
  auto liquidity_ind = core::fix::FillLiquidityInd::UNDEFINED;
  auto liquidity_ind_found = false;
  for (auto &item : execution_report.no_fills) {
    if (item.fill_liquidity_ind != core::fix::FillLiquidityInd::UNDEFINED) {
      if (!liquidity_ind_found) {
        liquidity_ind = item.fill_liquidity_ind;
        liquidity_ind_found = true;
      } else if (item.fill_liquidity_ind != liquidity_ind) {
        liquidity_ind = core::fix::FillLiquidityInd::UNDEFINED;
        break;
      }
    }
  }
  // note! https://stackoverflow.com/a/46115028
  const auto &exec_type = execution_report.exec_type;
  const auto &ord_status = execution_report.ord_status;
  if (!flags::Common::disable_deribit_143()) {
    // - partial fill could overlap cancel request (#143)
    if (exec_type == core::fix::ExecType::CANCELED &&
        ord_status == core::fix::OrdStatus::CANCELED) {
      log::warn<1>("Drop execution report due to FIX compliance"sv);
      return;
    }
  }
  const auto &orig_cl_ord_id = execution_report.orig_cl_ord_id;
  const auto &text = execution_report.text;
  const auto &no_fills = execution_report.no_fills;
  const auto &transact_time = execution_report.transact_time;
  // find order
  auto side = core::fix::map(execution_report.side);
  auto order_status = core::fix::map(execution_report.ord_status);
  auto order_type = core::fix::map(execution_report.ord_type);
  auto last_liquidity = core::fix::map(liquidity_ind);
  // XXX TODO(thraneh): exec_inst
  auto request_type = compute_request_type(exec_type, ord_status);
  auto request_status = compute_request_status(exec_type, ord_status);
  auto [last_traded_quantity, last_traded_price] = compute_last_traded(
      execution_report.last_qty, execution_report.last_px, execution_report.no_fills);
  // note!
  // we have very little information to match requests as we can't rewrite ClOrdID
  // - create and modify both have exec_type=ORDER_STATUS and ord_status=NEW
  // - reject has nothing
  oms::Response response{
      .type = request_type,
      .origin = Origin::EXCHANGE,
      .status = request_status,
      .error = fix::map_error(text),
      .text = text,
      .version = {},
      .request_id = {},
      .quantity = execution_report.order_qty,
      .price = execution_report.price,
  };
  oms::OrderUpdate order_update{
      .account = security_.get_account(),
      .exchange = flags::Config::exchange(),
      .symbol = execution_report.symbol,
      .side = side,
      .position_effect = {},
      .max_show_quantity = execution_report.max_show,
      .order_type = order_type,
      .time_in_force = {},
      .execution_instruction = {},  // XXX TODO(thraneh)
      .order_template = {},
      .create_time_utc = {},
      .update_time_utc = execution_report.transact_time,
      .external_account = {},
      .external_order_id = execution_report.order_id,
      .status = order_status,
      .quantity = execution_report.order_qty,
      .price = execution_report.price,
      .stop_price = execution_report.stop_px,
      .remaining_quantity = execution_report.leaves_qty,
      .traded_quantity = execution_report.cum_qty,
      .average_traded_price = execution_report.avg_px,
      .last_traded_quantity = last_traded_quantity,
      .last_traded_price = last_traded_price,
      .last_liquidity = last_liquidity,
  };
  if (download) {
    if (shared_.create_order(
            orig_cl_ord_id,  // note! *always* from create order (can't rewrite)
            stream_id_,
            trace_info,
            order_update)) {
    } else {
      auto external = std::empty(execution_report.deribit_label);
      if (external)
        log::warn("*** EXTERNAL ORDER ***"sv);
      else
        log::warn("*** UNKNOWN INTERNAL ORDER ***"sv);
      log::warn("execution_report={}"sv, execution_report);
    }
  } else {
    if (shared_.update_order(
            orig_cl_ord_id,  // note! *always* from create order (can't rewrite)
            stream_id_,
            trace_info,
            response,
            order_update,
            [&](auto &order) {
              log::debug("found order={}"sv, order);
              core::back_emplacer fills(shared_.fills);
              for (auto &item : no_fills) {
                fills.emplace_back([&](auto &result) { emplace(result, item); });
              }
              if (!std::empty(fills)) {
                TradeUpdate trade_update{
                    .stream_id = stream_id_,
                    .account = order.account,
                    .order_id = order.order_id,
                    .exchange = order.exchange,
                    .symbol = order.symbol,
                    .side = order.side,
                    .position_effect = order.position_effect,
                    .create_time_utc = transact_time,
                    .update_time_utc = transact_time,
                    .external_account = order.external_account,
                    .external_order_id = order.external_order_id,
                    .fills = fills,
                    .routing_id = order.routing_id,
                };
                server::create_trace_and_dispatch(
                    handler_, trace_info, trade_update, true, order.user_id);
              }
            })) {
    } else {
      auto external = std::empty(execution_report.deribit_label);
      if (external)
        log::warn("*** EXTERNAL ORDER ***"sv);
      else
        log::warn("*** UNKNOWN INTERNAL ORDER ***"sv);
      log::warn("execution_report={}"sv, execution_report);
    }
  }
  // download end?
  download_.check_relaxed(OrderEntryState::ORDERS);
}

void OrderEntry::operator()(
    const core::fix::Event<fix::OrderCancelReject> &event, const server::TraceInfo &trace_info) {
  auto &[header, order_cancel_reject] = event;
  log::warn<1>("event={{header={}, order_cancel_reject={}}}"sv, header, order_cancel_reject);
  const auto &ord_status = order_cancel_reject.ord_status;
  const auto &text = order_cancel_reject.text;
  oms::Response response{
      .type = {},  // modify or cancel
      .origin = Origin::EXCHANGE,
      .status = RequestStatus::REJECTED,
      .error = fix::map_error(text),
      .text = text,
      .version = {},
      .request_id = {},
      .quantity = NaN,
      .price = NaN,
  };
  if (shared_.update_order(
          order_cancel_reject.orig_cl_ord_id, stream_id_, trace_info, response, [&](auto &order) {
            log::debug("found order={}"sv, order);
            auto status = core::fix::map(ord_status);
            if (status != order.status) {
              log::warn(
                  "Unexpected: order status received={}, expected={}"sv, status, order.status);
            }
          })) {
  } else {
    log::warn("*** EXTERNAL ORDER ***"sv);
    log::warn("order_cancel_reject={}"sv, order_cancel_reject);
  }
}

namespace {
RequestType message_type_to_request_type(core::fix::MsgType msg_type) {
  switch (msg_type) {
    case core::fix::MsgType::NEW_ORDER_SINGLE:
      return RequestType::CREATE_ORDER;
    case core::fix::MsgType::ORDER_CANCEL_REPLACE_REQUEST:
      return RequestType::MODIFY_ORDER;
    case core::fix::MsgType::ORDER_CANCEL_REQUEST:
      return RequestType::CANCEL_ORDER;
    default:
      return {};
  }
}
}  // namespace

void OrderEntry::operator()(
    const core::fix::Event<fix::Reject> &event, const server::TraceInfo &trace_info) {
  auto &[header, reject] = event;
  log::warn<1>("event={{header={}, reject={}}}"sv, header, reject);
  auto request_type = message_type_to_request_type(reject.ref_msg_type);
  if (request_type != RequestType{}) {
    auto iter = msg_seq_num_to_request_id_.find(reject.ref_seq_num);
    if (iter != std::end(msg_seq_num_to_request_id_)) {
      auto &request_id = (*iter).second;
      auto error = fix::reject_to_error(reject.session_reject_reason, reject.text);
      oms::Response response{
          .type = request_type,
          .origin = Origin::EXCHANGE,
          .status = RequestStatus::REJECTED,
          .error = error,
          .text = reject.text,
          .version = {},
          .request_id = request_id,
          .quantity = NaN,
          .price = NaN,
      };
      if (shared_.update_order(
              request_id, stream_id_, trace_info, response, []([[maybe_unused]] auto &order) {})) {
      } else {
        log::warn<1>(R"(*** NO ORDER WITH REQUEST_ID="{}" ***)"sv, request_id);
      }
    } else {
      log::warn<1>(R"(*** NO REQUEST FOR MSG_SEQ_NUM="{}" ***)"sv, reject.ref_seq_num);
    }
  } else if (
      reject.session_reject_reason.compare("99"sv) == 0 &&
      reject.text.compare("connection_too_slow"sv) == 0) {
    log::info("closing connection"sv);
    connection_.close();
  } else {
    log::fatal("Unexpected"sv);
  }
}

void OrderEntry::operator()(
    const core::fix::Event<fix::OrderMassCancelReport> &event, const server::TraceInfo &) {
  auto &[header, order_mass_cancel_report] = event;
  log::info<1>(
      "event={{header={}, order_mass_cancel_report={}}}"sv, header, order_mass_cancel_report);
  switch (order_mass_cancel_report.mass_cancel_response) {
    case core::fix::MassCancelResponse::CANCEL_REQUEST_REJECTED:
      log::warn(
          R"(*** CANCEL ALL ORDERS FAILED, REASON="{}" ***)"sv,
          order_mass_cancel_report.mass_cancel_reject_reason);
      break;
    default:
      log::info(
          "*** CANCEL ALL ORDERS SUCCEEDED, TOTAL_AFFECTED_ORDERS={} ***"sv,
          order_mass_cancel_report.total_affected_orders);
  }
}

template <typename T>
uint64_t OrderEntry::send(const T &event) {
  return send(event, core::get_realtime_clock());
}

template <typename T>
uint64_t OrderEntry::send(const T &event, std::chrono::nanoseconds sending_time) {
  core::fix::Writer writer(
      encode_buffer_,
      FIX_VERSION,
      T::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      outbound_.msg_seq_num,
      sending_time);
  auto message = event.encode(writer);
  log::info<0>::when(flags::FIX::fix_debug(), "{}"sv, core::fix::Debug(message));
  connection_.send(message);
  return outbound_.msg_seq_num;
}

void OrderEntry::check(const core::fix::header_t &header) {
  auto current = header.msg_seq_num;
  auto expected = inbound_.msg_seq_num + 1;
  if (current != expected) [[unlikely]] {
    if (expected < current) {
      log::warn(
          "*** SEQUENCE GAP *** "
          "current={} previous={} distance={}"sv,
          current,
          inbound_.msg_seq_num,
          current - inbound_.msg_seq_num);
    } else {
      log::warn(
          "*** SEQUENCE REPLAY *** "
          "current={} previous={} distance={}"sv,
          current,
          inbound_.msg_seq_num,
          inbound_.msg_seq_num - current);
    }
  }
  inbound_.msg_seq_num = current;
}

}  // namespace deribit
}  // namespace roq
