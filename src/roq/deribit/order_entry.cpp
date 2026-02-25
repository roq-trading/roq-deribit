/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/order_entry.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/charconv/from_chars.hpp"
#include "roq/utils/charconv/to_string.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/utils/debug/fix/message.hpp"
#include "roq/utils/debug/hex/message.hpp"

#include "roq/fix/map.hpp"
#include "roq/fix/reader.hpp"

#include "roq/deribit/common.hpp"

#include "roq/deribit/fix/utils.hpp"

// business (outbound)
#include "roq/deribit/fix/new_order_single.hpp"
#include "roq/deribit/fix/order_cancel_replace_request.hpp"
#include "roq/deribit/fix/order_cancel_request.hpp"
#include "roq/deribit/fix/order_mass_cancel_request.hpp"
#include "roq/deribit/fix/order_mass_status_request.hpp"
#include "roq/deribit/fix/request_for_positions.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === CONSTANTS ===

namespace {
auto const NAME = "om"sv;

auto const SUPPORTS = Mask{
    SupportType::CREATE_ORDER,
    SupportType::MODIFY_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
    SupportType::ORDER,
    SupportType::TRADE,
    SupportType::POSITION,
};

auto const LOGOUT_RESPONSE = "LOGOUT"sv;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id, auto const &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account);
}

auto create_connection_factory(auto &settings, auto &context) {
  auto uri = settings.fix.uri;
  auto config = io::net::ConnectionFactory::Config{
      .interface = settings.misc.test_local_interface,
      .uris = {&uri, 1},
      .validate_certificate = settings.net.tls_validate_certificate,
  };
  return io::net::ConnectionFactory::create(context, config);
}

auto create_connection_manager(auto &handler, auto &settings, auto &connection_factory) {
  auto config = io::net::ConnectionManager::Config{
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = {},
      .always_reconnect = true,
  };
  return io::net::ConnectionManager::create(handler, connection_factory, config);
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

OrderEntry::OrderEntry(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account.name)},
      connection_factory_{create_connection_factory(shared.settings, context)},
      connection_manager_{create_connection_manager(*this, shared.settings, *connection_factory_)}, decode_buffer_(shared.settings.misc.decode_buffer_size),
      encode_buffer_2_(shared.settings.misc.encode_buffer_size),
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .position_report = create_metrics(shared.settings, name_, "position_report"sv),
          .execution_report = create_metrics(shared.settings, name_, "execution_report"sv),
          .order_cancel_reject = create_metrics(shared.settings, name_, "order_cancel_reject"sv),
          .reject = create_metrics(shared.settings, name_, "reject"sv),
          .order_mass_cancel_report = create_metrics(shared.settings, name_, "order_mass_cancel_report"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      account_{account}, shared_{shared}, download_{shared.settings.fix.request_timeout, [this](auto state) { return download(state); }} {
}

void OrderEntry::operator()(Event<Start> const &) {
  (*connection_manager_).start();
}

void OrderEntry::operator()(Event<Stop> const &) {
  (*connection_manager_).stop();
}

void OrderEntry::operator()(Event<Timer> const &event) {
  if (!(*connection_manager_).refresh(event.value.now)) {
    return;
  }
  if (last_logon_or_heartbeat_.count() != 0 && shared_.settings.fix.request_timeout.count() != 0 &&
      (event.value.now - last_logon_or_heartbeat_) > shared_.settings.fix.request_timeout) {
    log::warn("*** DETECTED TIMEOUT ***"sv);
    log::info("DEBUG: now={}, last_logon_or_heartbeat={}"sv, event.value.now, last_logon_or_heartbeat_);
    log::info("closing connection"sv);
    (*connection_manager_).close();
  } else {
    if (ready_) {
      if (test_disconnect_time_.count() != 0 && test_disconnect_time_ < event.value.now) [[unlikely]] {
        if (shared_.settings.fix.test_order_disconnect.count() != 0) {
          log::warn("*** TEST: DISCONNECT ***"sv);
          log::info("closing connection"sv);
          (*connection_manager_).close();
        }
      } else {
        if (next_heartbeat_ <= event.value.now) {
          assert(shared_.settings.fix.ping_freq.count() > 0);
          next_heartbeat_ = event.value.now + shared_.settings.fix.ping_freq;
          send_test_request(clock::get_system());
        }
      }
    } else {
      if (test_logon_time_.count() != 0 && test_logon_time_ < event.value.now) {
        if (shared_.settings.fix.test_order_logon.count() != 0) {
          log::warn("*** TEST: LOGON ***"sv);
        }
        test_logon_time_ = {};
        send_logon();
        (*this)(ConnectionStatus::LOGIN_SENT);
      }
    }
  }
}

uint16_t OrderEntry::operator()(
    Event<CreateOrder> const &event, server::oms::Order const &, server::oms::RefData const &ref_data, std::string_view const &request_id) {
  if (!ready()) [[unlikely]] {
    throw server::oms::NotReady{"not ready"sv};
  }
  auto &[message_info, create_order] = event;
  if (std::isfinite(create_order.stop_price)) [[unlikely]] {
    throw RuntimeError{"stop_price not supported"sv};
  }
  if (std::isfinite(create_order.max_show_quantity)) [[unlikely]] {
    throw RuntimeError{"max_show_quantity not supported"sv};
  }
  auto side = map(create_order.side);
  auto exec_inst = fix::map(create_order.execution_instructions);
  auto ord_type = map(create_order.order_type);
  auto time_in_force = map(create_order.time_in_force);
  auto new_order_single = fix::NewOrderSingle{
      .cl_ord_id = request_id,
      .side = side,
      .order_qty = {create_order.quantity, ref_data.quantity.precision},
      .price = {create_order.price, ref_data.price.precision},
      .symbol = create_order.symbol,
      .exec_inst = exec_inst,
      .ord_type = ord_type,
      .time_in_force = time_in_force,
      .stop_px = {create_order.stop_price, ref_data.price.precision},
      .deribit_label = request_id,
      .deribit_adv_order_type = '\0',
      .deribit_mm_protection = {},
      .deribit_condition_trigger_method = {},
  };
  auto msg_seq_num = send(new_order_single);
  // XXX HANS EXPERIMENTAL -- it's a leak / currently no way to clean up
  msg_seq_num_to_request_id_.emplace(msg_seq_num, request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    Event<ModifyOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  if (!ready()) [[unlikely]] {
    throw server::oms::NotReady{"not ready"sv};
  }
  auto &modify_order = event.value;
  auto side = map(order.side);
  auto ord_type = map(order.order_type);
  // note! using deribit_label might be slower, but using orig_cl_ord_id seems error-prone
  auto order_cancel_replace_request = fix::OrderCancelReplaceRequest{
      .cl_ord_id = {},
      .orig_cl_ord_id = {},
      .deribit_label = order.client_order_id,
      .symbol = order.symbol,
      .currency = {},
      .side = side,
      .order_qty = {modify_order.quantity, ref_data.quantity.precision},
      .ord_type = ord_type,
      .price = {modify_order.price, ref_data.price.precision},
      .exec_inst = {},
      .deribit_mm_protection = {},
  };
  auto msg_seq_num = send(order_cancel_replace_request);
  // XXX HANS EXPERIMENTAL -- it's a leak / currently no way to clean up
  msg_seq_num_to_request_id_.emplace(msg_seq_num, request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    Event<CancelOrder> const &,
    server::oms::Order const &order,
    server::oms::RefData const &,
    std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  if (!ready()) [[unlikely]] {
    throw server::oms::NotReady{"not ready"sv};
  }
  // note! using deribit_label might be slower, but using orig_cl_ord_id seems error-prone
  auto order_cancel_request = fix::OrderCancelRequest{
      .cl_ord_id = {},
      .orig_cl_ord_id = {},
      .deribit_label = order.client_order_id,
      .symbol = order.symbol,
      .currency = {},
  };
  auto msg_seq_num = send(order_cancel_request);
  // XXX HANS EXPERIMENTAL -- it's a leak / currently no way to clean up
  msg_seq_num_to_request_id_.emplace(msg_seq_num, request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  if (!ready()) [[unlikely]] {
    throw server::oms::NotReady{"not ready"sv};
  }
  auto &[message_info, cancel_all_orders] = event;
  auto send_ack = [&]() {
    auto cancel_all_orders_ack = CancelAllOrdersAck{
        .stream_id = stream_id_,
        .account = account_.name,
        .order_id = cancel_all_orders.order_id,
        .exchange = cancel_all_orders.exchange,
        .symbol = cancel_all_orders.symbol,
        .side = cancel_all_orders.side,
        .origin = Origin::GATEWAY,
        .request_status = RequestStatus::FORWARDED,
        .error = {},
        .text = {},
        .request_id = request_id,
        .external_account = {},
        .number_of_affected_orders = {},
        .round_trip_latency = {},
        .user = {},
        .strategy_id = cancel_all_orders.strategy_id,
    };
    TraceInfo trace_info{event};
    Trace event_2{trace_info, cancel_all_orders_ack};
    shared_(event_2);
  };
  auto filter = utils::create_filter(cancel_all_orders) & ~Mask{Filter::ACCOUNT};
  auto mass_cancel_request_type = [&]() {
    if (std::empty(filter)) {
      return roq::fix::MassCancelRequestType::CANCEL_ALL_ORDERS;
    }
    // note! no need to validate exchange
    if (!std::empty(filter & Mask{Filter::SYMBOL}) && std::empty(filter & ~Mask{Filter::EXCHANGE, Filter::SYMBOL})) {
      return roq::fix::MassCancelRequestType::CANCEL_ORDERS_FOR_SECURITY;
    }
    log::warn("DEBUG: filter={}"sv, filter);
    throw server::oms::Rejected{Origin::GATEWAY, Error::INVALID_FILTER, "filter"sv};
  }();
  auto order_mass_cancel_request = fix::OrderMassCancelRequest{
      .cl_ord_id = request_id,
      .mass_cancel_request_type = mass_cancel_request_type,
      .security_type = {},
      .symbol = cancel_all_orders.symbol,
      .currency = {},
  };
  log::warn("DEBUG: order_mass_cancel_request={}"sv, order_mass_cancel_request);
  send(order_mass_cancel_request);
  send_ack();
  return stream_id_;
}

void OrderEntry::operator()(metrics::Writer &writer) const {
  writer  //
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.execution_report, metrics::Type::PROFILE)
      .write(profile_.order_cancel_reject, metrics::Type::PROFILE)
      .write(profile_.reject, metrics::Type::PROFILE)
      .write(profile_.order_mass_cancel_report, metrics::Type::PROFILE)
      .write(latency_.ping, metrics::Type::LATENCY);
}

void OrderEntry::operator()(io::net::ConnectionManager::Connected const &) {
  assert(test_logon_time_.count() == 0);
  auto now = clock::get_system();
  test_logon_time_ = now + shared_.settings.fix.test_order_logon;
  if (shared_.settings.fix.test_order_disconnect.count() != 0) {
    test_disconnect_time_ = now + shared_.settings.fix.test_order_disconnect;
  }
}

void OrderEntry::operator()(io::net::ConnectionManager::Disconnected const &) {
  ++counter_.disconnect;
  outbound_ = {};
  inbound_ = {};
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
  last_logon_or_heartbeat_ = {};
  // test
  test_logon_time_ = {};
  test_disconnect_time_ = {};
}

void OrderEntry::operator()(io::net::ConnectionManager::Read const &) {
  auto buffer = (*connection_manager_).buffer();
  size_t total_bytes = 0;
  while (!std::empty(buffer)) {
    TraceInfo trace_info;
    auto bytes = roq::fix::Reader<FIX_VERSION>::dispatch(
        buffer,
        [&](roq::fix::Message const &message) {
          try {
            check(message.header);
            Trace event{trace_info, message};
            parse(event);
          } catch (std::exception &) {
            log::warn("{}"sv, utils::debug::fix::Message{buffer});
#ifndef NDEBUG
            log::warn("{}"sv, utils::debug::hex::Message{buffer});
#endif
            if (!shared_.settings.fix.continue_from_parse_exception) [[likely]] {
              throw;
            } else {
              log::error("Message could not be parsed. PLEASE REPORT!"sv);
            }
          }
        },
        [this](auto &message) {
          if (shared_.settings.fix.debug) {
            log::info("{}"sv, utils::debug::fix::Message{message});
          }
        });
    if (bytes == 0) {
      break;
    }
    assert(bytes <= std::size(buffer));
    total_bytes += bytes;
    buffer = buffer.subspan(bytes);
  }
  (*connection_manager_).drain(total_bytes);
}

void OrderEntry::operator()(io::net::ConnectionManager::Write const &) {
}

void OrderEntry::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = account_.name,
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::FIX,
      .encoding = {Encoding::FIX},
      .priority = Priority::PRIMARY,
      .connection_status = connection_status_,
      .reason = reason,
      .interface = (*connection_factory_).get_interface(),
      .authority = (*connection_factory_).get_current_authority(),
      .path = (*connection_factory_).get_current_path(),
      .proxy = {},
  };
  log::info("stream_status={}"sv, stream_status);
  create_trace_and_dispatch(handler_, trace_info, stream_status);
}

void OrderEntry::send_logon() {
  auto ping_freq = std::chrono::duration_cast<std::chrono::seconds>(shared_.settings.fix.ping_freq);
  auto now = clock::get_realtime<std::chrono::milliseconds>();
  auto raw_data = account_.create_raw_data(now);
  auto password = account_.create_password(raw_data);
  auto logon = fix::Logon{
      .heart_bt_int = static_cast<uint16_t>(ping_freq.count()),
      .raw_data_length = static_cast<uint32_t>(std::size(raw_data)),
      .raw_data = raw_data,
      .username = account_.key,
      .password = password,
      .use_wordsafe_tags = false,
      .cancel_on_disconnect = shared_.settings.fix.cancel_on_disconnect,
      .deribit_app_id = {},
      .deribit_app_sig = {},
      .deribit_sequential = false,
      .unsubscribe_execution_reports = false,
  };
  send(logon);
  last_logon_or_heartbeat_ = clock::get_system();
}

void OrderEntry::send_logout(std::string_view const &text) {
  auto logout = fix::Logout{
      .text = text,
  };
  send(logout);
}

void OrderEntry::send_heartbeat(std::string_view const &test_req_id) {
  auto heartbeat = fix::Heartbeat{
      .test_req_id = test_req_id,
  };
  send(heartbeat);
}

void OrderEntry::send_test_request(std::chrono::nanoseconds now) {
  // request_id is current time
  encode_buffer_.clear();
  utils::charconv::to_string(std::back_inserter(encode_buffer_), now.count());
  auto test_request = fix::TestRequest{
      .test_req_id = encode_buffer_,
  };
  send(test_request);
  if (last_logon_or_heartbeat_.count() == 0) {
    last_logon_or_heartbeat_ = now;
  }
}

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    using enum OrderEntryState;
    case UNDEFINED:
      assert(false);
      break;
    case POSITIONS:
      (*this)(ConnectionStatus::DOWNLOADING, "positions"sv);
      subscribe_positions();
      return 1;
    case ORDERS:
      download_orders();
      (*this)(ConnectionStatus::DOWNLOADING, "orders"sv);
      return 1;  // note! first report includes the true number of reports
    case DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return 0;
  }
  assert(false);
  return 0;
}

void OrderEntry::subscribe_positions() {
  auto request_id = shared_.next_request_id();
  auto request_for_positions = fix::RequestForPositions{
      .pos_req_id = request_id,
      .pos_req_type = roq::fix::PosReqType::POSITIONS,
      .subscription_request_type = roq::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
      .currency = {},
  };
  send(request_for_positions);
}

void OrderEntry::download_orders() {
  auto request_id = shared_.next_request_id();
  auto order_mass_status_request = fix::OrderMassStatusRequest{
      .mass_status_req_id = request_id,
      .mass_status_req_type = roq::fix::MassStatusReqType::ORDERS,
  };
  send(order_mass_status_request);
}

void OrderEntry::parse(Trace<roq::fix::Message> const &event) {
  profile_.parse([&]() { parse_helper(event); });
}

void OrderEntry::parse_helper(Trace<roq::fix::Message> const &event) {
  auto &trace_info = event.trace_info;
  auto &message = event.value;
  switch (message.header.msg_type) {
    using enum roq::fix::MsgType;
    // session
    case HEARTBEAT: {
      auto heartbeat = fix::Heartbeat::create(message);
      create_trace_and_dispatch(*this, trace_info, heartbeat, message.header);
      return;
    }
    case LOGON: {
      auto logon = fix::Logon::create(message);
      create_trace_and_dispatch(*this, trace_info, logon, message.header);
      return;
    }
    case LOGOUT: {
      auto logout = fix::Logout::create(message);
      create_trace_and_dispatch(*this, trace_info, logout, message.header);
      return;
    }
    case RESEND_REQUEST: {
      auto resend_request = fix::ResendRequest::create(message);
      create_trace_and_dispatch(*this, trace_info, resend_request, message.header);
      return;
    }
    case TEST_REQUEST: {
      auto test_request = fix::TestRequest::create(message);
      create_trace_and_dispatch(*this, trace_info, test_request, message.header);
      return;
    }
    // ...
    case POSITION_REPORT: {
      profile_.position_report([&]() {
        auto position_report = fix::PositionReport::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, position_report, message.header);
      });
      return;
    }
    case EXECUTION_REPORT: {
      profile_.execution_report([&]() {
        auto execution_report = fix::ExecutionReport::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, execution_report, message.header);
      });
      return;
    }
    case ORDER_CANCEL_REJECT: {
      profile_.order_cancel_reject([&]() {
        auto order_cancel_reject = fix::OrderCancelReject::create(message);
        create_trace_and_dispatch(*this, trace_info, order_cancel_reject, message.header);
      });
      return;
    }
    case REJECT: {
      profile_.reject([&]() {
        auto reject = fix::Reject::create(message);
        create_trace_and_dispatch(*this, trace_info, reject, message.header);
      });
      return;
    }
    case ORDER_MASS_CANCEL_REPORT: {
      profile_.order_mass_cancel_report([&]() {
        auto order_mass_cancel_report = fix::OrderMassCancelReport::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, order_mass_cancel_report, message.header);
      });
      return;
    }
    default:
      break;
  }
  log::warn("Unexpected msg_type={}"sv, message.header.msg_type);
}

void OrderEntry::operator()(Trace<fix::Heartbeat> const &event, roq::fix::Header const &header) {
  auto now = clock::get_system();
  auto &[trace_info, heartbeat] = event;
  log::info<3>("event={{header={}, heartbeat={}}}"sv, header, heartbeat);
  last_logon_or_heartbeat_ = {};
  if (!std::empty(heartbeat.test_req_id)) {
    auto send_time = std::chrono::nanoseconds{utils::charconv::from_chars<uint64_t>(heartbeat.test_req_id)};
    auto latency = (now - send_time) / 2;  // 1-way
    auto external_latency = ExternalLatency{
        .stream_id = stream_id_,
        .account = account_.name,
        .latency = latency,
    };
    create_trace_and_dispatch(handler_, trace_info, external_latency);
    latency_.ping.update(latency);
  }
}

void OrderEntry::operator()(Trace<fix::Logon> const &event, roq::fix::Header const &header) {
  auto &[trace_info, logon] = event;
  log::info<2>("event={{header={}, logon={}}}"sv, header, logon);
  last_logon_or_heartbeat_ = {};
  download_.begin();
}

void OrderEntry::operator()(Trace<fix::Logout> const &event, roq::fix::Header const &header) {
  auto &[trace_info, logout] = event;
  log::warn("event={{header={}, logout={}}}"sv, header, logout);
  ready_ = false;
  // note! mandated, must send a logout response
  send_logout(LOGOUT_RESPONSE);
  log::info("closing connection"sv);
  (*connection_manager_).close();
}

void OrderEntry::operator()(Trace<fix::ResendRequest> const &event, roq::fix::Header const &header) {
  auto &[trace_info, resend_request] = event;
  log::warn("event={{header={}, resend_request={}}}"sv, header, resend_request);
  log::info("closing connection"sv);
  (*connection_manager_).close();
}

void OrderEntry::operator()(Trace<fix::TestRequest> const &event, roq::fix::Header const &header) {
  auto &[trace_info, test_request] = event;
  log::info<1>("event={{header={}, test_request={}}}"sv, header, test_request);
  send_heartbeat(test_request.test_req_id);
}

void OrderEntry::operator()(Trace<fix::PositionReport> const &event, roq::fix::Header const &header) {
  auto &[trace_info, position_report] = event;
  log::info<2>("event={{header={}, position_report={}}}"sv, header, position_report);
  for (size_t i = 0; i < std::size(position_report.no_positions); ++i) {
    auto is_last = std::size(position_report.no_positions) == (i + 1);
    auto &position_qty = position_report.no_positions[i];
    // XXX FIXME we only get pos_type which is always Transaction Quantity ==> margin mode = isolated?
    auto long_quantity = std::max(0.0, position_qty.long_qty);
    auto short_quantity = std::max(0.0, position_qty.short_qty);
    auto position_update = PositionUpdate{
        .stream_id = stream_id_,
        .account = account_.name,
        .exchange = shared_.settings.exchange,
        .symbol = position_qty.symbol,
        .margin_mode = {},  // XXX FIXME TODO
        .external_account = {},
        .long_quantity = long_quantity,
        .short_quantity = short_quantity,
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(handler_, trace_info, position_update, is_last);
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

RequestType compute_request_type(auto const exec_type, auto const ord_status) {
  switch (exec_type) {
    using enum roq::fix::ExecType;
    case REJECTED:
      return {};  // any
    case CANCELED:
      return {};  // could be IOC or FOK
    case ORDER_STATUS:
      switch (ord_status) {
        using enum roq::fix::OrdStatus;
        case NEW:
        case PARTIALLY_FILLED:
          return {};  // create or modify
        case CANCELED:
          return {};
          break;
        default:
          break;
      }
    default:
      break;
  }
  return {};
}

RequestStatus compute_request_status(auto const exec_type, auto const ord_status) {
  switch (exec_type) {
    using enum roq::fix::ExecType;
    case REJECTED:
      return RequestStatus::REJECTED;
    case CANCELED:
      return RequestStatus::ACCEPTED;
    case ORDER_STATUS:
      switch (ord_status) {
        using enum roq::fix::OrdStatus;
        case NEW:
        case PARTIALLY_FILLED:
        case FILLED:
        case CANCELED:
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

auto find_liquidity_ind(auto const &fills) {
  auto result = roq::fix::FillLiquidityInd::UNDEFINED;
  auto found = false;
  for (auto &item : fills) {
    if (item.fill_liquidity_ind != roq::fix::FillLiquidityInd::UNDEFINED) {
      if (!found) {
        result = item.fill_liquidity_ind;
        found = true;
      } else if (item.fill_liquidity_ind != result) {
        result = roq::fix::FillLiquidityInd::UNDEFINED;
        break;
      }
    }
  }
  return result;
}

// note!
//   last traded is expected (downstream) to be the sum of all fills for this update
//   Deribit reports only the *last* fill, but includes all fills as well
//   we will therefore replace these values, when possible
std::pair<double, double> compute_last_traded(auto const last_traded_quantity, auto const last_traded_price, auto const &fills) {
  if (std::empty(fills)) {
    return {last_traded_quantity, last_traded_price};
  }
  double sum_quantity = 0.0;
  double sum_quantity_price = 0.0;
  for (auto &item : fills) {
    sum_quantity += item.fill_qty;
    sum_quantity_price += item.fill_qty * item.fill_px;
  }
  auto average_price = utils::is_zero(sum_quantity) ? NaN : sum_quantity_price / sum_quantity;
  return {sum_quantity, average_price};
}

UpdateType compute_update_type(auto const &download) {
  if (download.state() != OrderEntryState::ORDERS) {
    return UpdateType::INCREMENTAL;
  }
  return UpdateType::SNAPSHOT;
}
}  // namespace

void OrderEntry::operator()(Trace<fix::ExecutionReport> const &event, roq::fix::Header const &header) {
  auto &[trace_info, execution_report] = event;
  log::info<2>("event={{header={}, execution_report={}}}"sv, header, execution_report);
  // download begin?
  switch (execution_report.mass_status_req_type) {
    using enum roq::fix::MassStatusReqType;
    case UNDEFINED:
      assert(std::empty(execution_report.mass_status_req_id));
      break;
    case ORDERS: {
      auto count = execution_report.tot_num_reports;
      log::info<1>(R"(Downloading {} execution reports (request_id="{}")"sv, count, execution_report.mass_status_req_id);
      download_.update(OrderEntryState::ORDERS, count);
      return;  // this is not an ordinary execution report
    }
    default:
      log::warn("execution_report={}"sv, execution_report);
      log::fatal(
          R"(Unexpected: mass_status_req_type={}, mass_status_req_id="{}")"sv, execution_report.mass_status_req_type, execution_report.mass_status_req_id);
      break;
  }
  // convenience
  auto exec_type = execution_report.exec_type;
  auto ord_status = execution_report.ord_status;
  // special case: partial fill can overlap cancel request (#143)
  if (!shared_.settings.misc.disable_deribit_143) {
    if (exec_type == roq::fix::ExecType::CANCELED && ord_status == roq::fix::OrdStatus::CANCELED) {
      log::warn<1>("Drop execution report due to FIX compliance"sv);
      return;
    }
  }
  auto request_or_exchange_id = [&]() {
    // note! rejects does not send orig_cl_ord_id
    if (std::empty(execution_report.orig_cl_ord_id)) {
      return execution_report.order_id;
    }
    return execution_report.orig_cl_ord_id;
  }();
  auto side = map(execution_report.side);
  auto order_status = map(execution_report.ord_status);
  auto order_type = map(execution_report.ord_type);
  auto average_traded_price = [&]() {
    if (std::isnan(execution_report.cum_qty) || utils::is_zero(execution_report.cum_qty)) {
      return NaN;
    }
    return execution_report.avg_px;
  }();
  auto liquidity_ind = find_liquidity_ind(execution_report.no_fills);
  auto last_liquidity = map(liquidity_ind);
  auto request_type = compute_request_type(exec_type, ord_status);
  auto request_status = compute_request_status(exec_type, ord_status);
  auto error = fix::map_error(execution_report.text);
  auto [last_traded_quantity, last_traded_price] = compute_last_traded(execution_report.last_qty, execution_report.last_px, execution_report.no_fills);
  auto update_type = compute_update_type(download_);
  auto time_in_force = [&]() -> TimeInForce {
    switch (update_type) {
      using enum UpdateType;
      case UNDEFINED:
        break;
      case SNAPSHOT:
        return TimeInForce::GTC;
      case INCREMENTAL:
        break;
      case STALE:
        break;
    }
    return {};
  }();
  // note!
  // we have very little information to match requests as we can't rewrite ClOrdID
  // - create and modify both have exec_type=ORDER_STATUS and ord_status=NEW
  // - reject has nothing
  auto response = server::oms::Response{
      .request_type = request_type,
      .origin = Origin::EXCHANGE,
      .request_status = request_status,
      .error = error,
      .text = execution_report.text,
      .version = {},
      .request_id = {},
      .external_order_id = {},
      .quantity = execution_report.order_qty,
      .price = execution_report.price,
  };
  auto order_update = server::oms::OrderUpdate{
      .account = account_.name,
      .exchange = shared_.settings.exchange,
      .symbol = execution_report.symbol,
      .side = side,
      .position_effect = {},
      .margin_mode = {},  // XXX FIXME TODO
      .max_show_quantity = execution_report.max_show,
      .order_type = order_type,
      .time_in_force = time_in_force,
      .execution_instructions = {},
      .create_time_utc = {},
      .update_time_utc = execution_report.transact_time,
      .external_account = {},
      .external_order_id = execution_report.order_id,
      .client_order_id = execution_report.deribit_label,
      .order_status = order_status,
      .error = {},
      .text = {},
      .quantity = execution_report.order_qty,
      .price = execution_report.price,
      .stop_price = execution_report.stop_px,
      .leverage = NaN,
      .remaining_quantity = execution_report.leaves_qty,
      .traded_quantity = execution_report.cum_qty,
      .average_traded_price = average_traded_price,
      .last_traded_quantity = last_traded_quantity,
      .last_traded_price = last_traded_price,
      .last_liquidity = last_liquidity,
      .routing_id = {},
      .max_request_version = {},
      .max_response_version = {},
      .max_accepted_version = {},
      .update_type = update_type,
      .sending_time_utc = header.sending_time,
  };
  auto user_id = SOURCE_NONE;
  auto order_id = ORDER_ID_NONE;
  auto strategy_id = STRATEGY_ID_NONE;
  if (shared_.update_order(request_or_exchange_id, stream_id_, trace_info, response, order_update, [&](auto &order) {
        user_id = order.user_id;
        order_id = order.order_id;
        strategy_id = order.strategy_id;
      })) {
  } else {
    auto external = std::empty(execution_report.deribit_label);
    if (external) {
      log::warn("*** EXTERNAL ORDER ***"sv);
    } else {
      log::warn("*** UNKNOWN INTERNAL ORDER ***"sv);
    }
    log::warn("execution_report={}"sv, execution_report);
  }
  if (!std::empty(execution_report.no_fills)) {
    auto &fills = shared_.get_fills();
    auto ref_data = shared_.get_ref_data(shared_.settings.exchange, execution_report.symbol);
    for (auto &item : execution_report.no_fills) {
      auto liquidity = map(item.fill_liquidity_ind);
      auto profit_loss_amount = utils::compute_profit_loss_amount(side, item.fill_qty, item.fill_px, ref_data.multiplier);
      auto fill = Fill{
          .exchange_time_utc = execution_report.transact_time,
          .external_trade_id = item.fill_exec_id,
          .quantity = item.fill_qty,
          .price = item.fill_px,
          .liquidity = liquidity,
          .commission_amount = NaN,  // note! we only have it per TRADE
          .commission_currency = {},
          .base_amount = NaN,
          .quote_amount = NaN,
          .profit_loss_amount = profit_loss_amount,
      };
      log::debug("fill={}"sv, fill);
      fills.emplace_back(fill);  // XXX FIXME std::move
    }
    assert(!std::empty(fills));
    auto trade_update = TradeUpdate{
        .stream_id = stream_id_,
        .account = account_.name,
        .order_id = order_id,
        .exchange = shared_.settings.exchange,
        .symbol = execution_report.symbol,
        .side = side,
        .position_effect = {},
        .margin_mode = {},  // XXX FIXME TODO
        .create_time_utc = execution_report.transact_time,
        .update_time_utc = execution_report.transact_time,
        .external_account = {},
        .external_order_id = execution_report.order_id,
        .client_order_id = {},
        .fills = fills,
        .routing_id = {},
        .update_type = update_type,
        .sending_time_utc = header.sending_time,
        .user = {},
        .strategy_id = strategy_id,
    };
    create_trace_and_dispatch(handler_, trace_info, trade_update, true, user_id, execution_report.deribit_label);
  }
  // download end?
  download_.check_relaxed(OrderEntryState::ORDERS);
}

void OrderEntry::operator()(Trace<fix::OrderCancelReject> const &event, roq::fix::Header const &header) {
  auto &[trace_info, order_cancel_reject] = event;
  log::warn<1>("event={{header={}, order_cancel_reject={}}}"sv, header, order_cancel_reject);
  auto error = fix::map_error(order_cancel_reject.text);
  auto response = server::oms::Response{
      .request_type = {},  // modify or cancel
      .origin = Origin::EXCHANGE,
      .request_status = RequestStatus::REJECTED,
      .error = error,
      .text = order_cancel_reject.text,
      .version = {},
      .request_id = {},
      .external_order_id = {},
      .quantity = NaN,
      .price = NaN,
  };
  auto request_or_exchange_id = [&]() {
    if (!std::empty(order_cancel_reject.deribit_label)) {
      return order_cancel_reject.deribit_label;
    }
    return order_cancel_reject.orig_cl_ord_id;
  }();
  if (shared_.update_order(request_or_exchange_id, stream_id_, trace_info, response, [&](auto &order) {
        OrderStatus status = map(order_cancel_reject.ord_status);
        if (status != order.order_status) {
          log::warn("Unexpected: order status received={}, expected={}"sv, status, order.order_status);
        }
      })) {
  } else {
    log::warn("*** EXTERNAL ORDER ***"sv);
    log::warn("order_cancel_reject={}"sv, order_cancel_reject);
  }
}

namespace {
RequestType message_type_to_request_type(auto const msg_type) {
  switch (msg_type) {
    using enum roq::fix::MsgType;
    case NEW_ORDER_SINGLE:
      return RequestType::CREATE_ORDER;
    case ORDER_CANCEL_REPLACE_REQUEST:
      return RequestType::MODIFY_ORDER;
    case ORDER_CANCEL_REQUEST:
      return RequestType::CANCEL_ORDER;
    default:
      return {};
  }
}
}  // namespace

void OrderEntry::operator()(Trace<fix::Reject> const &event, roq::fix::Header const &header) {
  auto &[trace_info, reject] = event;
  log::warn<1>("event={{header={}, reject={}}}"sv, header, reject);
  auto request_type = message_type_to_request_type(reject.ref_msg_type);
  if (request_type != RequestType{}) {
    auto iter = msg_seq_num_to_request_id_.find(reject.ref_seq_num);
    if (iter != std::end(msg_seq_num_to_request_id_)) {
      auto &request_id = (*iter).second;
      auto error = fix::reject_to_error(reject.session_reject_reason, reject.text);
      auto response = server::oms::Response{
          .request_type = request_type,
          .origin = Origin::EXCHANGE,
          .request_status = RequestStatus::REJECTED,
          .error = error,
          .text = reject.text,
          .version = {},
          .request_id = request_id,
          .external_order_id = {},
          .quantity = NaN,
          .price = NaN,
      };
      if (shared_.update_order(request_id, stream_id_, trace_info, response, []([[maybe_unused]] auto &order) {})) {
      } else {
        log::warn<1>(R"(*** NO ORDER WITH REQUEST_ID="{}" ***)"sv, request_id);
      }
    } else {
      log::warn<1>(R"(*** NO REQUEST FOR MSG_SEQ_NUM="{}" ***)"sv, reject.ref_seq_num);
    }
  } else if (reject.session_reject_reason == "99"sv && reject.text == "connection_too_slow"sv) {
    log::warn(R"(closing connection (reason: "{}"))"sv, reject.text);
    (*connection_manager_).close();
  } else if (reject.ref_msg_type == roq::fix::MsgType::ORDER_MASS_CANCEL_REQUEST && reject.text == "rate_limit_exceeded"sv) {
    // ???
    log::warn(R"(closing connection (reason: "{}"))"sv, reject.text);
    (*connection_manager_).close();
  } else {
    log::fatal("Unexpected: reject={}"sv, reject);
    // log::warn("Unexpected: reject={}"sv, reject);
    // download_.check_relaxed(OrderEntryState::ORDERS);
  }
}

void OrderEntry::operator()(Trace<fix::OrderMassCancelReport> const &event, roq::fix::Header const &header) {
  auto &[trace_info, order_mass_cancel_report] = event;
  log::info<1>("event={{header={}, order_mass_cancel_report={}}}"sv, header, order_mass_cancel_report);
  switch (order_mass_cancel_report.mass_cancel_response) {
    using enum roq::fix::MassCancelResponse;
    case CANCEL_REQUEST_REJECTED:
      log::warn(R"(*** CANCEL ALL ORDERS FAILED, REASON="{}" ***)"sv, order_mass_cancel_report.mass_cancel_reject_reason);
      break;
    default:
      log::info("*** CANCEL ALL ORDERS SUCCEEDED, TOTAL_AFFECTED_ORDERS={} ***"sv, order_mass_cancel_report.total_affected_orders);
  }
  auto status = [&]() {
    switch (order_mass_cancel_report.mass_cancel_response) {
      using enum roq::fix::MassCancelResponse;
      case CANCEL_REQUEST_REJECTED:
        return RequestStatus::REJECTED;
      default:
        return RequestStatus::ACCEPTED;
    }
  }();
  auto cancel_all_orders_ack = CancelAllOrdersAck{
      .stream_id = stream_id_,
      .account = account_.name,
      .order_id = {},
      .exchange = {},
      .symbol = {},
      .side = {},
      .origin = Origin::EXCHANGE,
      .request_status = status,
      .error = {},
      .text = order_mass_cancel_report.text,
      .request_id = order_mass_cancel_report.cl_ord_id,
      .external_account = {},
      .number_of_affected_orders = order_mass_cancel_report.total_affected_orders,
      .round_trip_latency = {},
      .user = {},
      .strategy_id = {},
  };
  Trace event_2{trace_info, cancel_all_orders_ack};
  shared_(event_2);
}

template <typename T>
uint64_t OrderEntry::send(T const &event) {
  auto now = clock::get_realtime();
  return send(event, now);
}

template <typename T>
uint64_t OrderEntry::send(T const &event, std::chrono::nanoseconds sending_time) {
  auto helper = [&](auto &message) { log::info("{}"sv, utils::debug::fix::Message{message}); };
  auto header = roq::fix::Header{
      .version = FIX_VERSION,
      .msg_type = T::MSG_TYPE,
      .sender_comp_id = SENDER_COMP_ID,
      .target_comp_id = TARGET_COMP_ID,
      .msg_seq_num = ++outbound_.msg_seq_num,  // note!
      .sending_time = sending_time,
  };
  if ((*connection_manager_).send([&](auto &buffer) {
        auto message = event.encode(header, buffer);
        if (shared_.settings.fix.debug) [[unlikely]] {
          helper(message);
        }
        return std::size(message);
      })) {
  } else {
    log::warn("HERE"sv);
  }
  return outbound_.msg_seq_num;
}

void OrderEntry::check(roq::fix::Header const &header) {
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
