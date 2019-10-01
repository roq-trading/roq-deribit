/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <gflags/gflags.h>

#include <limits>

#include "roq/logging.h"
#include "roq/stream.h"

#include "roq/core/clock.h"

#include "roq/core/fix/utils.h"

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

DEFINE_string(fix_uri, "tcp://test.deribit.com:9881", "FIX end-point (URI)");
DEFINE_uint64(ping_freq_secs, uint64_t{10}, "ping frequency (seconds)");
DEFINE_string(exchange, "DERIBIT", "exchange identifier (string)");
DEFINE_bool(cancel_on_disconnect, true, "cancel orders on disconnect? (bool)");

DEFINE_int32(network_affinity, -1, "network (epoll) affinity");

namespace roq {
namespace deribit {

namespace {
constexpr auto DECODE_BUFFER_SIZE = size_t{1024 * 4096};  // FIXME(thraneh): flag
constexpr auto ENCODE_BUFFER_SIZE = size_t{256 * 4096};  // FIXME(thraneh): flag
constexpr auto TIMER_FREQUENCY = std::chrono::milliseconds{100};
constexpr auto ACCOUNT = "A1";  // FIXME(thraneh): flag
// constexpr auto SYMBOL = "BTC-27DEC19";  // DEBUG
constexpr auto MAX_DEPTH = size_t{65536};  // FIXME(thraneh): HACK !!!
}  // namespace

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const Config& config)
    : _dispatcher(dispatcher),
      _dns_base(_base, true),
      _timer(_base, EV_PERSIST, [this]() { on_timer(); }),
      _decode_buffer(DECODE_BUFFER_SIZE),
      _encode_buffer(ENCODE_BUFFER_SIZE),
      // fix:
      _fix(
          *this,
          _ssl_context,
          _base,
          _dns_base,
          core::URI(FLAGS_fix_uri)),
      _access_key(config.get_access_key()),
      _access_secret(config.get_access_secret()),
      _symbols(config.symbols) {
}

void Gateway::on(const StartEvent& event) {
  LOG(INFO) << "Starting the gateway event loop...";
  _thread = std::thread([this]() { run(); });
}

void Gateway::on(const StopEvent& event) {
  LOG(INFO) << "Stopping the gateway event loop...";
  _stop.store(true, std::memory_order_release);
  if (_thread.joinable())
    _thread.join();
  LOG(INFO) << "The gateway event loop has stopped";
}

void Gateway::on(const TimerEvent& event) {
}

void Gateway::on(const ConnectionStatusEvent& event) {
}

void Gateway::on(const CreateOrderEvent& event) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::on(const ModifyOrderEvent& event) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::on(const CancelOrderEvent& event) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::write(Metrics& metrics) const {
  // TODO(thraneh): _latency
}

void Gateway::run() {
  LOG(INFO) << "Gateway event loop has started";
  try {
    initialize_thread();
    _timer.add(TIMER_FREQUENCY);
    _fix.start();
    _base.loop(EVLOOP_NO_EXIT_ON_EMPTY);
  } catch (std::exception& e) {
    LOG(FATAL) << "Unhandled exception, what=\"" << e.what() << "\"";
  } catch (...) {
    LOG(FATAL) << "Unhandled exception";
  }
  LOG(INFO) << "Gateway event loop has finished";
}

void Gateway::initialize_thread() {
  if (FLAGS_network_affinity >= 0) {
    LOG(INFO) << "Thread affinity " << FLAGS_network_affinity;
    set_thread_affinity(FLAGS_network_affinity);
  }
}

void Gateway::on_timer() {
  if (_stop.load(std::memory_order_acquire)) {
    _base.loopbreak();
    return;
  }
  auto now = core::get_time();
  if (now < _next_update)
    return;
  _next_update = now + std::chrono::seconds{FLAGS_ping_freq_secs};
  switch (_gateway_status) {
    case GatewayStatus::DOWNLOADING:
    case GatewayStatus::READY: {
      VLOG(4) << "Sending FIX TestRequest";
      auto test_req_id = fmt::format(
          "{}",
          core::get_system_clock().count());
      fix::TestRequest test_request {
        .test_req_id = test_req_id,
      };
      send(test_request);
      break;
    }
    default:
      break;
  }
}

// FIX:

void Gateway::on_fix_connected() {
  assert(_gateway_status == GatewayStatus::DISCONNECTED);
  LOG(INFO) << fmt::format(
      "[FIX] request logon (username=\"{}\")...", _access_key);
  auto sending_time = core::get_realtime_clock();
  auto raw_data = Random::create_raw_data(sending_time);
  auto password = Random::create_password(raw_data, _access_secret);
  fix::Logon logon = {
    .heart_bt_int = static_cast<uint16_t>(FLAGS_ping_freq_secs),
    .raw_data = raw_data,
    .username = _access_key,
    .password = password,
    .deribit_cancel_on_disconnect = FLAGS_cancel_on_disconnect,
  };
  send(logon, sending_time);
  _gateway_status = GatewayStatus::LOGIN_SENT;
  MarketDataStatus market_data_status = {
    .status = _gateway_status,
  };
  enqueue(market_data_status, true);
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = _gateway_status,
  };
  enqueue(order_manager_status, true);
}

void Gateway::on_fix_disconnected() {
  reset();
  assert(_gateway_status != GatewayStatus::DISCONNECTED);
  _gateway_status = GatewayStatus::DISCONNECTED;
  MarketDataStatus market_data_status = {
    .status = _gateway_status,
  };
  enqueue(market_data_status, true);
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = _gateway_status,
  };
  enqueue(order_manager_status, true);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::ExecutionReport& execution_report) {
  VLOG(1) << fmt::format(
      "header={}, execution_report={}",
      header,
      execution_report);
  switch (execution_report.exec_type) {
    case core::fix::ExecType::ORDER_STATUS:
      // TODO(thraneh): forward to gateway
      /*
      // DEBUG
      switch (execution_report.ord_status){
        case core::fix::OrdStatus::NEW:
          if (execution_report.order_qty > 1.0) {
            fix::OrderCancelReplaceRequest order_cancel_replace_request = {
              .cl_ord_id = execution_report.orig_cl_ord_id,  // TODO(thraneh): check order
              .orig_cl_ord_id = execution_report.cl_ord_id,
              .side = core::fix::Side::BUY,
              .order_qty = 1.0,
              .ord_type = core::fix::OrdType::LIMIT,
              .price = 1.0,
              .symbol = SYMBOL,
              .transact_time = execution_report.transact_time,
            };
            send(order_cancel_replace_request);
          } else {
            fix::OrderCancelRequest order_cancel_request = {
              .cl_ord_id = execution_report.orig_cl_ord_id,  // TODO(thraneh): check order
              .orig_cl_ord_id = execution_report.cl_ord_id,
            };
            send(order_cancel_request);
          }
          break;
        default:
          break;
      }
      */
      break;
    default:
      break;
  }
  // download?
  switch (execution_report.mass_status_req_type) {
    case core::fix::MassStatusReqType::ORDERS:
      _download_execution_reports = execution_report.tot_num_reports;
      if (_download_execution_reports == 0) {
        LOG(INFO) << fmt::format(
            "[FIX] download orders COMPLETED (len(orders)={})",
          execution_report.tot_num_reports);
        process();
      } else {
        LOG(INFO) << fmt::format(
            "[FIX] download orders IN PROGRESS (len(orders)={})",
            execution_report.tot_num_reports);
      }
      break;
    default:
      if (_download_execution_reports &&
          0 == --_download_execution_reports)
        process();
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Heartbeat& heartbeat) {
  // note! get clock *before* any logging (avoid latency)
  auto now = core::get_system_clock();
  VLOG(3) << fmt::format(
      "header={}, heartbeat={}",
      header,
      heartbeat);
  if (heartbeat.test_req_id.empty() == false) {
    auto send_time = core::charconv::from_string<uint64_t>(
        heartbeat.test_req_id);
    _latency = now - decltype(now){send_time};
    VLOG(1) << fmt::format(
        "[FIX] latency={}",
        std::chrono::duration_cast<std::chrono::microseconds>(
          _latency));
  }
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Logon& logon) {
  VLOG(1) << fmt::format(
      "header={}, logon={}",
      header,
      logon);
  LOG(INFO) << "[FIX] logon COMPLETED";
  process(true);
  /*
  // DEBUG
  fix::NewOrderSingle new_order_single = {
    .cl_ord_id = "roq-ord-006",
    .side = core::fix::Side::BUY,
    .order_qty = 2.0,
    .price = 0.5,
    .symbol = SYMBOL,
    .ord_type = core::fix::OrdType::LIMIT,
    .time_in_force = core::fix::TimeInForce::GTC,
    .deribit_label = "roq;123;345",
  };
  send(new_order_single);
  */
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Logout& logout) {
  VLOG(1) << fmt::format(
      "header={}, logout={}",
      header,
      logout);
  LOG(WARNING) << fmt::format(
      "[FIX] logout (text=\"{}\")",
      logout.text);
  reset();
  MarketDataStatus market_data_status = {
    .status = _gateway_status,
  };
  enqueue(market_data_status, true);
  OrderManagerStatus order_manager_status = {
    .account = ACCOUNT,
    .status = GatewayStatus::LOGGED_OUT,
  };
  enqueue(order_manager_status, true);
  // note! mandated, must send a logout response
  fix::Logout response = {
    .text = "i'm done",
  };
  send(response);
  LOG(FATAL) << "Unexpected -- now what?";  // FIXME(thraneh): ...
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
  assert(market_data_incremental_refresh.symbol.empty() == false);
  assert(market_data_incremental_refresh.symbol.length() < 32);
  // TODO(thraneh): speak to Deribit ...
  if (market_data_incremental_refresh.md_inc_grp.length == 0)
    return;
  VLOG(3) << fmt::format(
      "header={}, market_data_incremental_refresh={}",
      header,
      market_data_incremental_refresh);
  /*
  MBPUpdate bid[MAX_DEPTH];
  MBPUpdate ask[MAX_DEPTH];
  size_t bid_length = 0, ask_length = 0;
  */
  std::vector<MBPUpdate> bid, ask;
  bid.reserve(MAX_DEPTH);
  ask.reserve(MAX_DEPTH);

  std::chrono::nanoseconds exchange_time_utc = {};
  auto& md_inc_grp = market_data_incremental_refresh.md_inc_grp;
  for (size_t i = 0; i < md_inc_grp.length; ++i) {
    auto& item = md_inc_grp.items[i];
    if (item.md_entry_date > exchange_time_utc)
      exchange_time_utc = item.md_entry_date;
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        /*
        fprintf(stderr, "%d [%zu/%zu] BID: %zu/%zu\n", __LINE__, i, md_inc_grp.length, ask_length, std::size(ask));
        new (&bid[bid_length]) MBPUpdate {
          .price = item.md_entry_px,
          .quantity = item.md_entry_size,
          .action = core::fix::map(item.md_update_action),
        };
        LOG_IF(FATAL, ++bid_length >= std::size(bid)) <<
          "Exceeding allocated space for bids";
          */
        bid.emplace_back(
            MBPUpdate {
              .price = item.md_entry_px,
              .quantity = item.md_entry_size,
              .action = core::fix::map(item.md_update_action),
            });
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        /*
        fprintf(stderr, "%d [%zu/%zu] ASK: %zu/%zu\n", __LINE__, i, md_inc_grp.length, ask_length, std::size(ask));
        new (&ask[ask_length]) MBPUpdate {
          .price = item.md_entry_px,
          .quantity = item.md_entry_size,
          .action = core::fix::map(item.md_update_action),
        };
        LOG_IF(FATAL, ++ask_length >= std::size(ask)) <<
          "Exceeding allocated space for asks";
        */
        ask.emplace_back(
            MBPUpdate {
              .price = item.md_entry_px,
              .quantity = item.md_entry_size,
              .action = core::fix::map(item.md_update_action),
            });
        break;
      }
      case core::fix::MDEntryType::TRADE: {
        TradeSummary trade_summary = {
          .exchange = FLAGS_exchange,
          .symbol = market_data_incremental_refresh.symbol,
          .price = item.md_entry_px,
          .volume = item.md_entry_size,
          .turnover = std::numeric_limits<double>::quiet_NaN(),
          .side = core::fix::map(item.side),
          .exchange_time_utc = exchange_time_utc,
        };
        // LOG(INFO) << "XXX=" << trade_summary;
        enqueue(trade_summary, true);  // FIXME(thraneh): *not* always last
        break;
      }
      case core::fix::MDEntryType::INDEX_VALUE:
      case core::fix::MDEntryType::SETTLEMENT_PRICE:
        // FIXME(thraneh): how to propagate these???
        break;
      default:
        LOG(WARNING) << fmt::format("[FIX] unsupported: {}", item);
        break;
    }
  }
  if (bid.empty() && ask.empty())
    return;
  MarketByPrice market_by_price = {
    .exchange = FLAGS_exchange,
    .symbol = market_data_incremental_refresh.symbol,
    .bid_length = bid.size(),
    .bid = bid.data(),
    .ask_length = ask.size(),
    .ask = ask.data(),
    .snapshot = false,
    .exchange_time_utc = exchange_time_utc,
  };
  // LOG(INFO) << "XXX=" << market_by_price;
  enqueue(market_by_price, true);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataRequestReject& market_data_request_reject) {
  VLOG(1) << fmt::format(
      "header={}, market_data_request_reject={}",
      header,
      market_data_request_reject);
  LOG(WARNING) << fmt::format(
      "[FIX] market data request reject (reason={}, text=\"{}\")",
      market_data_request_reject.md_req_rej_reason,
      market_data_request_reject.text);
  LOG(FATAL) << "Unexpected -- now what?";  // FIXME(thraneh): ...
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
  VLOG(3) << fmt::format(
      "header={}, market_data_snapshot_full_refresh={}",
      header,
      market_data_snapshot_full_refresh);
  /*
  MBPUpdate bid[MAX_DEPTH];
  MBPUpdate ask[MAX_DEPTH];
  */
  std::vector<MBPUpdate> bid, ask;
  bid.reserve(MAX_DEPTH);
  ask.reserve(MAX_DEPTH);

  auto& md_full_grp = market_data_snapshot_full_refresh.md_full_grp;
  for (size_t i = 0; i < md_full_grp.length; ++i) {
    auto& item = md_full_grp.items[i];
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        /*
        fprintf(stderr, "%d [%zu/%zu] BID: %zu/%zu\n", __LINE__, i, md_full_grp.length, market_by_price.bid_length, std::size(bid));
        new (&bid[market_by_price.bid_length]) MBPUpdate {
          .price = item.md_entry_px,
          .quantity = item.md_entry_size,
          .action = UpdateAction::NEW,
        };
        LOG_IF(FATAL, ++market_by_price.bid_length >= std::size(bid)) <<
          "Exceeding allocated space for bids";
          */
        bid.emplace_back(
            MBPUpdate {
              .price = item.md_entry_px,
              .quantity = item.md_entry_size,
              .action = UpdateAction::NEW,
            });
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        /*
        fprintf(stderr, "%d [%zu/%zu] ASK: %zu/%zu\n", __LINE__, i, md_full_grp.length, market_by_price.ask_length, std::size(ask));
        new (&ask[market_by_price.ask_length]) MBPUpdate {
          .price = item.md_entry_px,
          .quantity = item.md_entry_size,
          .action = UpdateAction::NEW,
        };
        LOG_IF(FATAL, ++market_by_price.ask_length >= std::size(ask)) <<
          "Exceeding allocated space for asks";
          */
        ask.emplace_back(
            MBPUpdate {
              .price = item.md_entry_px,
              .quantity = item.md_entry_size,
              .action = UpdateAction::NEW,
            });
        break;
      }
      default:
        break;
    }
  }
  MarketByPrice market_by_price = {
    .exchange = FLAGS_exchange,
    .symbol = market_data_snapshot_full_refresh.symbol,
    .bid_length = bid.size(),
    .bid = bid.data(),
    .ask_length = ask.size(),
    .ask = ask.data(),
    .snapshot = true,
    .exchange_time_utc = {},
  };
  enqueue(market_by_price, true);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::OrderCancelReject& order_cancel_reject) {
  VLOG(1) << fmt::format(
      "header={}, order_cancel_reject={}",
      header,
      order_cancel_reject);
  // TODO(thraneh): forward to gateway
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::PositionReport& position_report) {
  VLOG(1) << fmt::format(
      "header={}, position_report={}",
      header,
      position_report);
  LOG(INFO) << fmt::format(
      "[FIX] download positions COMPLETED (len(positions)={})",
      position_report.positions.length);
  // TODO(thraneh): forward to gateway
  process();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::Reject& reject) {
  VLOG(1) << fmt::format(
      "header={}, reject={}",
      header,
      reject);
  LOG(WARNING) << fmt::format(
      "[FIX] reject (msg_type=\"{}\", text=\"{}\")",
      reject.ref_msg_type,
      reject.text);
  LOG(FATAL) << "Unexpected -- now what?";  // FIXME(thraneh): ...
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::ResendRequest& resend_request) {
  VLOG(1) << fmt::format(
      "header={}, resend_request={}",
      header,
      resend_request);
  fix::Reject reject = {
    .ref_seq_num = header.msg_seq_num,
    .ref_tag_id = 0,
    .ref_msg_type = header.msg_type_raw,
    .text = "resend_not_supported",
  };
  send(reject);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::SecurityList& security_list) {
  VLOG(3) << fmt::format(
      "header={}, security_list={}",
      header,
      security_list);
  LOG(INFO) << fmt::format(
      "[FIX] download instruments COMPLETED (len(instruments)={})",
      security_list.instruments.length);
  if (security_list.instruments.length == 0)
    return;
  std::vector<std::string_view> symbols;
  symbols.reserve(security_list.instruments.length);
  for (size_t i = 0; i < security_list.instruments.length; ++i) {
    auto& instrument = security_list.instruments.items[i];
    if (discard_symbol(instrument.symbol))
      continue;
    ReferenceData reference_data = {
      .exchange = FLAGS_exchange,
      .symbol = instrument.symbol,
      .tick_size = instrument.min_price_increment,
      .limit_up = std::numeric_limits<double>::quiet_NaN(),
      .limit_down = std::numeric_limits<double>::quiet_NaN(),
      .multiplier = instrument.contract_multiplier,
    };
    enqueue(reference_data, true);
    MarketStatus market_status = {
      .exchange = FLAGS_exchange,
      .symbol = instrument.symbol,
      .trading_status = TradingStatus::OPEN,  // TODO(thraneh): no info from exch?
    };
    enqueue(market_status, true);
    /*
    if (symbols.size() > 9)  // 58
      continue;
    */
    symbols.emplace_back(instrument.symbol);
  }
  if (symbols.empty() == false) {
    fix::MarketDataRequest market_data_request = {
      .md_req_id = "roq-mkt-002",
      .symbols = symbols,
    };
    send(market_data_request);
  }
  process();
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::TestRequest& test_request) {
  VLOG(1) << fmt::format(
      "header={}, test_request={}",
      header,
      test_request);
  fix::Heartbeat heartbeat = {
    .test_req_id = test_request.test_req_id,
  };
  send(heartbeat);
}

void Gateway::operator()(
    const core::fix::header_t& header,
    const fix::UserResponse& user_response) {
  VLOG(1) << fmt::format(
      "header={}, user_response={}",
      header,
      user_response);
  LOG(INFO) << "[FIX] download user COMPLETED";
  // TODO(thraneh): forward to gateway
  process();
}

// UTILS:

inline bool Gateway::discard_symbol(const std::string_view& symbol) {
  for (auto& iter : _symbols)
    if (std::regex_match(symbol.begin(), symbol.end(), iter)) {
      return false;
    }
  VLOG(1) << fmt::format(
      "Discard symbol=\"{}\" (reason: no regex match)", symbol);
  return true;
}

void Gateway::process(bool initialize) {
  if (initialize) {
    assert(_download == Download::NONE);
    LOG(INFO) << "[FIX] download:";
    assert(_gateway_status == GatewayStatus::LOGIN_SENT);
    _gateway_status = GatewayStatus::DOWNLOADING;
    MarketDataStatus market_data_status = {
      .status = _gateway_status,
    };
    enqueue(market_data_status, true);
    OrderManagerStatus order_manager_status = {
      .account = ACCOUNT,
      .status = _gateway_status,
    };
    enqueue(order_manager_status, true);
    LOG(INFO) << "[FIX] download instruments...";
    fix::SecurityListRequest security_list_request = {
      .security_req_id = "download_securities",
    };
    send(security_list_request);
    _download = Download::SECURITIES;
  } else {
    switch (_download) {
      case Download::NONE:
        assert(false);
        break;
      case Download::SECURITIES: {
        LOG(INFO) << "[FIX] download positions...";
        fix::RequestForPositions request_for_positions = {
          .pos_req_id = "download_positions",
          .pos_req_type = core::fix::PosReqType::POSITIONS,
        };
        send(request_for_positions);
        _download = Download::POSITIONS;
        break;
      }
      case Download::POSITIONS: {
        LOG(INFO) << "[FIX] download orders...";
        fix::OrderMassStatusRequest order_mass_status_request = {
          .mass_status_req_id = "download_orders",
          .mass_status_req_type = core::fix::MassStatusReqType::ORDERS,
        };
        send(order_mass_status_request);
        _download = Download::ORDERS;
        break;
      }
      case Download::ORDERS: {
        LOG(INFO) << "[FIX] download user...";
        fix::UserRequest user_request = {
          .user_request_id = "download_user",
          .username = _access_key,
        };
        send(user_request);
        _download = Download::USER;
        break;
      }
      case Download::USER: {
        assert(_gateway_status == GatewayStatus::DOWNLOADING);
        _gateway_status = GatewayStatus::READY;
        MarketDataStatus market_data_status = {
          .status = _gateway_status,
        };
        enqueue(market_data_status, true);
        OrderManagerStatus order_manager_status = {
          .account = ACCOUNT,
          .status = _gateway_status,
        };
        enqueue(order_manager_status, true);
        LOG(INFO) << "[FIX] download COMPLETED";
        _download = Download::NONE;
        break;
      };
    }
  }
}

void Gateway::reset() {
  _download = Download::NONE;
}

}  // namespace deribit
}  // namespace roq
