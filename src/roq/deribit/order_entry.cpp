/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/order_entry.h"

#include "roq/utils/mask.h"
#include "roq/utils/safe_cast.h"
#include "roq/utils/update.h"

#include "roq/core/back_emplacer.h"
#include "roq/core/debug.h"

#include "roq/core/metrics/factory.h"

#include "roq/core/fix/debug.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/common.h"
#include "roq/deribit/flags.h"

#include "roq/deribit/fix/utils.h"

// business (outbound)
#include "roq/deribit/fix/new_order_single.h"
#include "roq/deribit/fix/order_cancel_replace_request.h"
#include "roq/deribit/fix/order_cancel_request.h"
#include "roq/deribit/fix/order_mass_cancel_request.h"
#include "roq/deribit/fix/order_mass_status_request.h"

using namespace roq::literals;

namespace roq {
namespace deribit {

namespace {
static const auto LOGOUT_RESPONSE = "LOGOUT"_sv;  // XXX

static const auto NAME = "om"_sv;
static const auto SUPPORTS = utils::Mask{
    SupportType::CREATE_ORDER,
    SupportType::MODIFY_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
    SupportType::ORDER,
    SupportType::TRADE,
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
      name_(fmt::format("{}:{}:{}"_sv, stream_id_, NAME, security.get_account())),
      connection_factory_(context, Flags::fix_uri()), connection_(*this, connection_factory_),
      encode_buffer_(Flags::encode_buffer_size()), decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"_sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"_sv),
          .execution_report = create_metrics(name_, "execution_report"_sv),
          .order_cancel_reject = create_metrics(name_, "order_cancel_reject"_sv),
          .reject = create_metrics(name_, "reject"_sv),
          .order_mass_cancel_report = create_metrics(name_, "order_mass_cancel_report"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
      },
      security_(security), shared_(shared),
      download_(Flags::fix_request_timeout(), [this](auto state) { return download(state); }) {
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
  if (last_logon_or_heartbeat_.count() && Flags::fix_request_timeout().count() &&
      (event.value.now - last_logon_or_heartbeat_) > Flags::fix_request_timeout()) {
    log::warn("*** DETECTED TIMEOUT ***"_sv);
    log::info("closing connection"_sv);
    connection_.close();
  } else {
    if (ready_ && next_heartbeat_ <= event.value.now) {
      assert(Flags::fix_ping_freq().count() > 0);
      next_heartbeat_ = event.value.now + Flags::fix_ping_freq();
      send_test_request(core::get_system_clock());
    }
  }
}

uint16_t OrderEntry::operator()(
    const Event<CreateOrder> &event, const std::string_view &request_id) {
  if (!ready())
    throw oms::NotReadyException();
  auto &[message_info, create_order] = event;
  if (std::isfinite(create_order.stop_price))
    throw RuntimeErrorException("stop_price not supported"_sv);
  if (std::isfinite(create_order.max_show_quantity))
    throw RuntimeErrorException("max_show_quantity not supported"_sv);
  auto side = core::fix::map(create_order.side);
  auto exec_inst = fix::map(create_order.execution_instruction);
  auto ord_type = core::fix::map(create_order.order_type);
  auto time_in_force = core::fix::map(create_order.time_in_force);
  core::stack::Buffer<char, MAX_LENGTH_REQUEST_ID> buffer;
  fmt::format_to(
      std::back_inserter(buffer), "roq-{}-{}"_sv, message_info.source, create_order.order_id);
  std::string_view deribit_label(buffer.data(), buffer.size());
  fix::NewOrderSingle new_order_single{
      .cl_ord_id = request_id,
      .side = side,
      .order_qty = create_order.quantity,
      .price = create_order.price,
      .symbol = create_order.symbol,
      .exec_inst = exec_inst,
      .ord_type = ord_type,
      .time_in_force = time_in_force,
      .deribit_label = deribit_label,
      .deribit_adv_order_type = '\0',
  };
  send(new_order_single);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    const Event<ModifyOrder> &event,
    const oms::Order &order,
    const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  if (!ready())
    throw oms::NotReadyException();
  const auto &modify_order = event.value;
  auto side = core::fix::map(order.side);
  auto ord_type = core::fix::map(order.order_type);
  fix::OrderCancelReplaceRequest order_cancel_replace_request{
      .orig_cl_ord_id = order.external_order_id,
      .cl_ord_id = request_id,
      .transact_time = utils::safe_cast(order.update_time_utc),
      .side = side,
      .order_qty = modify_order.quantity,
      .ord_type = ord_type,
      .price = modify_order.price,
      .symbol = order.symbol,
      .exec_inst = {},
  };
  send(order_cancel_replace_request);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    const Event<CancelOrder> &,
    const oms::Order &order,
    const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  if (!ready())
    throw oms::NotReadyException();
  fix::OrderCancelRequest order_cancel_request{
      .cl_ord_id = request_id,
      .orig_cl_ord_id = order.external_order_id,
  };
  send(order_cancel_request);
  send(order_cancel_request);
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
        R"(*** NOT CONNECTED! UNABLE TO CANCEL ALL ORDERS FOR ACCOUNT="{}")"_sv,
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
  while (!buffer.empty()) {
    auto bytes = core::fix::Reader<FIX_VERSION>::dispatch(
        [&](const core::fix::message_t &message) {
          try {
            check(message.header);
            parse(message);
          } catch (std::exception &) {
            log::warn("{}"_sv, core::fix::Debug(buffer));
#if !defined(NDEBUG)
            core::print_memory(buffer);
            core::print_string_with_escapes(buffer);
#endif
            throw;
          }
        },
        buffer,
        [](auto &message) {
          if (ROQ_UNLIKELY(Flags::fix_debug()))
            log::info("{}"_sv, core::fix::Debug(message));
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
    server::TraceInfo trace_info;
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::FIX,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"_sv, stream_status);
    server::create_trace_and_dispatch(trace_info, stream_status, handler_);
  }
}

void OrderEntry::send_logon() {
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
  auto request_id = std::string_view(stack_buffer_.data(), stack_buffer_.size());
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
  server::TraceInfo trace_info;
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
      log::warn("Unexpected msg_type={}"_sv, message.header.msg_type);
      break;
  }
}

void OrderEntry::operator()(
    const core::fix::Event<fix::Heartbeat> &event, const server::TraceInfo &) {
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  auto &[header, heartbeat] = event;
  log::info<3>("event(header={}, heartbeat={})"_sv, header, heartbeat);
  last_logon_or_heartbeat_ = {};
  if (!heartbeat.test_req_id.empty()) {
    auto send_time = core::from_chars<uint64_t>(heartbeat.test_req_id);
    auto latency =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - decltype(now){send_time}) /
        2;  // 1-way
    server::TraceInfo trace_info;
    ExternalLatency external_latency{
        .stream_id = stream_id_,
        .latency = latency,
    };
    server::create_trace_and_dispatch(trace_info, external_latency, handler_);
    latency_.ping.update(latency);
  }
}

void OrderEntry::operator()(const core::fix::Event<fix::Logon> &event, const server::TraceInfo &) {
  auto &[header, logon] = event;
  log::info<2>("event(header={}, logon={})"_sv, header, logon);
  last_logon_or_heartbeat_ = {};
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void OrderEntry::operator()(const core::fix::Event<fix::Logout> &event, const server::TraceInfo &) {
  auto &[header, logout] = event;
  log::warn<2>("event(header={}, logout={})"_sv, header, logout);
  ready_ = false;
  // note! mandated, must send a logout response
  send_logout(LOGOUT_RESPONSE);
  log::info("closing connection"_sv);
  connection_.close();
}

void OrderEntry::operator()(
    const core::fix::Event<fix::ResendRequest> &event, const server::TraceInfo &) {
  auto &[header, resend_request] = event;
  log::warn("event(header={}, resend_request={})"_sv, header, resend_request);
  log::info("closing connection"_sv);
  connection_.close();
}

void OrderEntry::operator()(
    const core::fix::Event<fix::TestRequest> &event, const server::TraceInfo &) {
  auto &[header, test_request] = event;
  log::info<1>("event(header={}, test_request={})"_sv, header, test_request);
  send_heartbeat(test_request.test_req_id);
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
        case core::fix::OrdStatus::CANCELED:
          return RequestStatus::ACCEPTED;
        case core::fix::OrdStatus::PARTIALLY_FILLED:
        case core::fix::OrdStatus::FILLED:
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return {};
}
}  // namespace

void OrderEntry::operator()(
    const core::fix::Event<fix::ExecutionReport> &event, const server::TraceInfo &trace_info) {
  auto &[header, execution_report] = event;
  log::info<3>("event(header={}, execution_report={})"_sv, header, execution_report);
  log::debug("execution_report={}"_sv, execution_report);
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
  const auto &orig_cl_ord_id = execution_report.orig_cl_ord_id;
  const auto &text = execution_report.text;
  const auto &no_fills = execution_report.no_fills;
  const auto &transact_time = execution_report.transact_time;
  // find order
  auto side = core::fix::map(execution_report.side);
  auto status = core::fix::map(execution_report.ord_status);
  auto order_type = core::fix::map(execution_report.ord_type);
  auto last_liquidity = core::fix::map(liquidity_ind);
  // XXX TODO(thraneh): exec_inst
  auto request_type = compute_request_type(exec_type, ord_status);
  auto request_status = compute_request_status(exec_type, ord_status);
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
      .exchange = Flags::exchange(),
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
      .status = status,
      .quantity = execution_report.order_qty,
      .price = execution_report.price,
      .stop_price = execution_report.stop_px,
      .remaining_quantity = execution_report.leaves_qty,
      .traded_quantity = execution_report.cum_qty,
      .average_traded_price = execution_report.avg_px,
      .last_traded_quantity = execution_report.last_qty,
      .last_traded_price = execution_report.last_px,
      .last_liquidity = last_liquidity,
  };
  if (download) {
    if (shared_.create_order(
            orig_cl_ord_id,  // note! *always* from create order (can't rewrite)
            stream_id_,
            trace_info,
            order_update)) {
    } else {
      auto external = execution_report.deribit_label.empty();
      log::warn(external ? "*** EXTERNAL ORDER ***"_sv : "*** UNKNOWN INTERNAL ORDER ***"_sv);
      log::warn("execution_report={}"_sv, execution_report);
    }
  } else {
    if (shared_.update_order(
            orig_cl_ord_id,  // note! *always* from create order (can't rewrite)
            stream_id_,
            trace_info,
            response,
            order_update,
            [&](auto &order) {
              log::debug("found order={}"_sv, order);
              core::back_emplacer fills(shared_.fills);
              for (auto &item : no_fills) {
                fills.emplace_back([&](auto &result) { emplace(result, item); });
              }
              if (!fills.empty()) {
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
                    trace_info, trade_update, handler_, true, order.user_id);
              }
            })) {
    } else {
      auto external = execution_report.deribit_label.empty();
      log::warn(external ? "*** EXTERNAL ORDER ***"_sv : "*** UNKNOWN INTERNAL ORDER ***"_sv);
      log::warn("execution_report={}"_sv, execution_report);
    }
  }
  // download end?
  download_.check_relaxed(OrderEntryState::ORDERS);
}

void OrderEntry::operator()(
    const core::fix::Event<fix::OrderCancelReject> &event, const server::TraceInfo &trace_info) {
  auto &[header, order_cancel_reject] = event;
  log::info<3>("event(header={}, order_cancel_reject={})"_sv, header, order_cancel_reject);
  log::debug("order_cancel_reject={}"_sv, order_cancel_reject);
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
            log::debug("found order={}"_sv, order);
            auto status = core::fix::map(ord_status);
            if (status != order.status) {
              log::warn(
                  "Unexpected: order status received={}, expected={}"_sv, status, order.status);
            }
          })) {
  } else {
    log::warn("*** EXTERNAL ORDER ***"_sv);
    log::warn("order_cancel_reject={}"_sv, order_cancel_reject);
  }
}

void OrderEntry::operator()(const core::fix::Event<fix::Reject> &event, const server::TraceInfo &) {
  auto &[header, reject] = event;
  log::info<3>("event(header={}, reject={})"_sv, header, reject);
  log::warn("reject={}"_sv, reject);
  if (reject.session_reject_reason.compare("99"_sv) == 0 &&
      reject.text.compare("connection_too_slow"_sv) == 0) {
    connection_.close();
  } else {
    log::fatal("Unexpected"_sv);
  }
}

void OrderEntry::operator()(
    const core::fix::Event<fix::OrderMassCancelReport> &event, const server::TraceInfo &) {
  auto &[header, order_mass_cancel_report] = event;
  log::info<1>(
      "event(header={}, order_mass_cancel_report={})"_sv, header, order_mass_cancel_report);
  switch (order_mass_cancel_report.mass_cancel_response) {
    case core::fix::MassCancelResponse::CANCEL_REQUEST_REJECTED:
      log::warn(
          R"(*** CANCEL ALL ORDERS FAILED, REASON="{}" ***)"_sv,
          order_mass_cancel_report.mass_cancel_reject_reason);
      break;
    default:
      log::info(
          "*** CANCEL ALL ORDERS SUCCEEDED, TOTAL_AFFECTED_ORDERS={} ***"_sv,
          order_mass_cancel_report.total_affected_orders);
  }
}

template <typename T>
void OrderEntry::send(const T &event) {
  send(event, core::get_realtime_clock());
}

template <typename T>
void OrderEntry::send(const T &event, std::chrono::nanoseconds sending_time) {
  core::fix::Writer writer(
      encode_buffer_,
      FIX_VERSION,
      T::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      outbound_.msg_seq_num,
      sending_time);
  auto message = event.encode(writer);
  if (ROQ_UNLIKELY(Flags::fix_debug()))
    log::info("{}"_sv, core::fix::Debug(message));
  connection_.send(message);
}

void OrderEntry::check(const core::fix::header_t &header) {
  auto current = header.msg_seq_num;
  auto expected = inbound_.msg_seq_num + 1;
  if (ROQ_UNLIKELY(current != expected)) {
    if (expected < current) {
      log::warn(
          "*** SEQUENCE GAP *** "
          "current={} previous={} distance={}"_sv,
          current,
          inbound_.msg_seq_num,
          current - inbound_.msg_seq_num);
    } else {
      log::warn(
          "*** SEQUENCE REPLAY *** "
          "current={} previous={} distance={}"_sv,
          current,
          inbound_.msg_seq_num,
          inbound_.msg_seq_num - current);
    }
  }
  inbound_.msg_seq_num = current;
}

}  // namespace deribit
}  // namespace roq
