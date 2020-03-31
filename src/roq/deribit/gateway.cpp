/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <limits>
#include <utility>

#include "roq/compat.h"

#include "roq/core/utils.h"

#include "roq/core/fix/utils.h"

#include "roq/deribit/options.h"

#include "roq/deribit/json/utils.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {

constexpr auto TOLERANCE = double{1.0e-10};

template <typename T>
static bool mbp_update(
    auto& data,
    size_t& offset,
    const T& item) {
  // validate
  switch (item.md_update_action) {
    case core::fix::MDUpdateAction::UNKNOWN:
      break;
    case core::fix::MDUpdateAction::NEW:
    case core::fix::MDUpdateAction::CHANGE:
      assert(std::fabs(item.md_entry_size) >= TOLERANCE);
      break;
    case core::fix::MDUpdateAction::DELETE:
      assert(std::fabs(item.md_entry_size) < TOLERANCE);
      break;
    case core::fix::MDUpdateAction::DELETE_THRU:
    case core::fix::MDUpdateAction::DELETE_FROM:
      throw std::runtime_error("MDUpdateAction not supported");
      break;
  }
  auto& obj = data[offset];
  new (&obj) MBPUpdate {
    .price = item.md_entry_px,
    .quantity = item.md_entry_size,
  };
  ++offset;
  return offset < data.size();
}

template <typename T>
static bool trade_update(
    auto& data,
    size_t& offset,
    const T& item) {
  auto& obj = data[offset];
  new (&obj) Trade {
    .side = core::fix::map(item.side),
    .price = item.md_entry_px,
    .quantity = item.md_entry_size,
    .trade_id = {},
  };
  core::copy_to(
      item.deribit_trade_id,
      obj.trade_id);
  ++offset;
  return offset < data.size();
}

template <typename T>
static bool fill_update(
    auto& dispatcher,
    auto& data,
    size_t& offset,
    const T& item) {
  auto trade_id = dispatcher.next_trade_id();
  auto& obj = data[offset];
  new (&obj) Fill {
    .quantity = item.fill_qty,
    .price = item.fill_px,
    .trade_id = trade_id,
    .gateway_trade_id = trade_id,
    .external_trade_id = {},
  };
  roq::core::copy_to(
      item.fill_exec_id,
      obj.external_trade_id);
  ++offset;
  return offset < data.size();
}

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const Config& config)
    : _dispatcher(dispatcher),
      _account(config.get_account()),
      _access_key(config.get_access_key()),
      _random(config.get_access_secret()),
      _dns_base(_base, true),
      _web_socket(
          *this,
          config,
          _random,
          _base,
          _dns_base,
          _ssl_context),
      _fix(
          *this,
          config,
          _random,
          _base,
          _dns_base),
      _fill(FLAGS_max_fills),
      _bid(FLAGS_max_depth),
      _ask(FLAGS_max_depth),
      _trade(FLAGS_max_trades) {
  LOG_IF(WARNING, FLAGS_cancel_on_disconnect == false)(
      "Orders will *NOT* be cancelled on disconnect");
}

void Gateway::operator()(const StartEvent& event) {
  LOG(INFO)("Starting the gateway...");
  _web_socket(event);
  _fix(event);
}

void Gateway::operator()(const StopEvent& event) {
  LOG(INFO)("Stopping the gateway...");
  _fix(event);
  _web_socket(event);
}

void Gateway::operator()(const TimerEvent& event) {
  if (_download != Download::NONE &&
      _download_timestamp.count() > 0 &&
      (event.now - _download_timestamp) >
          std::chrono::seconds { FLAGS_download_timeout_secs }) {
    LOG(WARNING)("Download time-out");
    _download_timestamp = {};
    _web_socket.close();
    _fix.close();
  } else {
    _web_socket(event);
    _fix(event);
  }
  _base.loop(EVLOOP_NONBLOCK);
}

void Gateway::operator()(const ConnectionStatusEvent&) {
}

void Gateway::operator()(
    const CreateOrderEvent& event,
    const std::string_view& request_id,
    uint32_t gateway_order_id) {
  auto& message_info = event.message_info;
  auto& create_order = event.create_order;
  if (std::isfinite(create_order.stop_price))
    throw std::runtime_error("stop_price not supported");
  if (std::isfinite(create_order.max_show_quantity))
    throw std::runtime_error("max_show_quantity not supported");
  core::stack::Buffer<char, 36> buffer;
  fmt::format_to(
      std::back_inserter(buffer),
      "roq-{}-{}-{}",
      gateway_order_id,
      message_info.source,
      create_order.order_id);
  std::string_view deribit_label(
      buffer.data(),
      buffer.size());
  fix::NewOrderSingle new_order_single {
    .cl_ord_id = request_id,
    .side = core::fix::map(create_order.side),
    .order_qty = create_order.quantity,
    .price = create_order.price,
    .symbol = create_order.symbol,
    .exec_inst = fix::map(create_order.execution_instruction),
    .ord_type = core::fix::map(create_order.order_type),
    .time_in_force = core::fix::map(create_order.time_in_force),
    .deribit_label = deribit_label,
    .deribit_adv_order_type = '\0',
  };
  _fix(new_order_single);
}

void Gateway::operator()(
    const ModifyOrderEvent& event,
    const std::string_view& request_id,
    const server::OMS_Order& order) {
  auto& modify_order = event.modify_order;
  fix::OrderCancelReplaceRequest order_cancel_replace_request {
    .orig_cl_ord_id = order.external_order_id,
    .cl_ord_id = request_id,
    .transact_time = order.update_time_utc,
    .side = core::fix::map(order.side),
    .order_qty = modify_order.quantity,
    .ord_type = core::fix::map(order.order_type),
    .price = modify_order.price,
    .symbol = order.symbol,
    .exec_inst = std::string_view(),
  };
  _fix(order_cancel_replace_request);
}

void Gateway::operator()(
    const CancelOrderEvent&,
    const std::string_view& request_id,
    const server::OMS_Order& order) {
  fix::OrderCancelRequest order_cancel_request {
    .cl_ord_id = request_id,
    .orig_cl_ord_id = order.external_order_id,
  };
  _fix(order_cancel_request);
}

void Gateway::operator()(Metrics& metrics) {
  _web_socket(metrics);
  _fix(metrics);
}

// web socket

void Gateway::operator()(const WebSocket&) {
  if (_web_socket.ready()) {
    // DEBUG
    _web_socket.get_currencies();
  }
}

void Gateway::operator()(const json::Currencies& currencies) {
  VLOG(1)(FMT_STRING("currencies={}"), currencies);
  for (auto& item : currencies.data) {
    _web_socket.get_instruments(item.currency);
    _web_socket.get_positions(item.currency);
  }
}

void Gateway::operator()(const json::Instruments& instruments) {
  VLOG(1)(FMT_STRING("instruments={}"), instruments);
  std::vector<std::string_view> symbols;
  symbols.reserve(instruments.data.size());
  for (auto& item : instruments.data) {
    if (_dispatcher.discard_symbol(item.instrument_name))
      continue;
    symbols.push_back(item.instrument_name);
  }
  if (symbols.empty())
    return;
  _web_socket.subscribe_ticker(
      roq::span(
          symbols.data(),
          symbols.size()));
}

void Gateway::operator()(const json::Positions& positions) {
  VLOG(1)(FMT_STRING("positions={}"), positions);
  // XXX check if we're done yet?
}

void Gateway::operator()(const json::Ticker& ticker) {
  VLOG(3)(FMT_STRING("ticker={}"), ticker);
  TopOfBook top_of_book = {
    .exchange = FLAGS_exchange,
    .symbol = ticker.instrument_name,
    .layer = {
      .bid_price = ticker.best_bid_price,
      .bid_quantity = ticker.best_bid_amount,
      .ask_price = ticker.best_ask_price,
      .ask_quantity = ticker.best_ask_amount,
    },
    .snapshot = false,
    .exchange_time_utc = ticker.timestamp,
  };
  enqueue(
      top_of_book,
      true);
  auto trading_status = json::map(ticker.state);
  auto& item = _trading_status[ticker.instrument_name];
  if (item != trading_status) {
    item = trading_status;
    MarketStatus market_status {
      .exchange = FLAGS_exchange,
      .symbol = ticker.instrument_name,
      .trading_status = json::map(ticker.state),
    };
    enqueue(
        market_status,
        true);
  }
}

// fix

void Gateway::operator()(const FIX& fix) {
  if (fix.ready()) {
    switch (_gateway_status) {
      case GatewayStatus::UNDEFINED:
        assert(false);
        break;
      case GatewayStatus::DISCONNECTED:
      case GatewayStatus::CONNECTING:
      case GatewayStatus::LOGIN_SENT:
        begin_download();
        break;
      case GatewayStatus::DOWNLOADING:
      case GatewayStatus::READY:
        break;
      case GatewayStatus::LOGGED_OUT:
        LOG(FATAL)("Unexpected");
    }
  } else {
    update(GatewayStatus::DISCONNECTED);
    reset();
  }
}

// execution_repot:
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
//   ORDER_STATUS    NEW                 ack success + order update
//   ORDER_STATUS    PARTIALLY_FILLED    order update
//   ORDER_STATUS    FILLED              order update
//   ORDER_STATUS    CANCELED            ack success

auto compute_request_status(
    auto request_type,
    auto exec_type,
    auto ord_status,
    bool download) {
  switch (exec_type) {
    case core::fix::ExecType::REJECTED: {
      switch (request_type) {
        case RequestType::UNDEFINED:
          LOG(WARNING)("*** EXTERNAL ACTION ***");
          break;
        case RequestType::CREATE_ORDER:
        case RequestType::MODIFY_ORDER:
        case RequestType::CANCEL_ORDER:
          return RequestStatus::REJECTED;
      }
      break;
    }
    case core::fix::ExecType::CANCELED: {
      switch (request_type) {
        case RequestType::UNDEFINED:
          LOG(WARNING)("*** EXTERNAL ACTION ***");
          break;
        case RequestType::CREATE_ORDER:
        case RequestType::MODIFY_ORDER:
          DLOG(FATAL)("UNEXPECTED");
          break;
        case RequestType::CANCEL_ORDER:
          return RequestStatus::ACCEPTED;
      }
      break;
    }
    case core::fix::ExecType::ORDER_STATUS:
      switch (ord_status) {
        case core::fix::OrdStatus::NEW: {
          switch (request_type) {
            case RequestType::UNDEFINED:
              LOG_IF(WARNING, download == false)("*** EXTERNAL ACTION ***");
              break;
            case RequestType::CREATE_ORDER:
            case RequestType::MODIFY_ORDER:
              return RequestStatus::ACCEPTED;
            case RequestType::CANCEL_ORDER:
              DLOG(FATAL)("UNEXPECTED");
              break;
          }
          break;
        }
        case core::fix::OrdStatus::PARTIALLY_FILLED:
        case core::fix::OrdStatus::FILLED:
          break;
        case core::fix::OrdStatus::CANCELED:
          switch (request_type) {
            case RequestType::UNDEFINED:
              break;
            case RequestType::CREATE_ORDER:
            case RequestType::MODIFY_ORDER:
              LOG(WARNING)("*** EXTERNAL ACTION ***");
              break;
            case RequestType::CANCEL_ORDER:
              return RequestStatus::ACCEPTED;
          }
      break;
        default:
          DLOG(FATAL)("UNEXPECTED");
          break;
      }
      break;
    default:
      DLOG(FATAL)("UNEXPECTED");
      break;
  }
  return RequestStatus::UNDEFINED;
}

void Gateway::operator()(
    const fix::ExecutionReport& execution_report) {
  // download begin?
  switch (execution_report.mass_status_req_type) {
    case core::fix::MassStatusReqType::ORDERS:
      assert(_gateway_status == GatewayStatus::DOWNLOADING);
      _download_execution_reports = execution_report.tot_num_reports;
      if (_download_execution_reports == 0)
        check_download(Download::ORDERS);
      return;
    default:
      break;
  }

  server::OMS_Lookup order_lookup {
    .symbol = execution_report.symbol,
    .side = core::fix::map(execution_report.side),
    .status = core::fix::map(execution_report.ord_status),
    .price = execution_report.price,
    .remaining_quantity = execution_report.leaves_qty,
    .traded_quantity = execution_report.cum_qty,
    // XXX note! this is per-trade -- need another approach
    // .commissions =  execution_report.commission,
    .commissions = std::numeric_limits<double>::quiet_NaN(),  // execution_report.commission,
    .timestamp = execution_report.transact_time,
    .external_order_id = execution_report.order_id,
  };

  // XXX we used to also create orders here...
  auto found = _dispatcher.find_order(
      execution_report.cl_ord_id,
      execution_report.orig_cl_ord_id,
      order_lookup,
      [&](const auto& order, auto& result) {
    result.request_status = compute_request_status(
        order.request_type,
        execution_report.exec_type,
        execution_report.ord_status,
        _download == Download::ORDERS);

    if (result.request_status != RequestStatus::UNDEFINED) {
      result.origin = Origin::EXCHANGE;
      result.error = fix::map_error(execution_report.text);
      result.text = execution_report.text;
    }

    size_t fill_length = 0;
    bool success = true;
    for (auto& item : execution_report.no_fills) {
      if (success == false)
        break;
      success = fill_update(
          _dispatcher,
          _fill,
          fill_length,
          item);
    }
    if (unlikely(success == false)) {
      LOG(FATAL)(
          FMT_STRING(
            "Insufficient fill array size(s): "
            "len(fill)={}/{}={}/{}"),
          fill_length, execution_report.no_fills.size());
    }
    if (fill_length) {
      TradeUpdate trade_update {
        .account = order.account,
        .order_id = order.user_order_id,
        .exchange = order.exchange,
        .symbol = order.symbol,
        .side = order.side,
        .position_effect = PositionEffect::UNDEFINED,
        .order_template = std::string_view(),
        .create_time_utc = execution_report.transact_time,
        .update_time_utc = execution_report.transact_time,
        .gateway_order_id = order.gateway_order_id,
        .external_order_id = order.external_order_id,
        .fills = roq::span<Fill const>(_fill.data(), fill_length),
      };
      enqueue(
          order.user_id,
          trade_update,
          true);
    }
  });

  // TODO(thraneh): process fills? --> maintain positions

  if (found == false) {
    auto external = execution_report.deribit_label.empty();
    LOG_IF(WARNING, external)("*** EXTERNAL ORDER ***");
    LOG_IF(WARNING, !external)("*** UNKNOWN INTERNAL ORDER ***");
  }

  // download end?
  if (_download_execution_reports &&
      0 == --_download_execution_reports) {
    check_download(Download::ORDERS);
  }
}

void Gateway::operator()(
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
  assert(_gateway_status == GatewayStatus::READY);
  bool success = true;
  size_t bid_length = 0, ask_length = 0, trade_length = 0;
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto& item : market_data_incremental_refresh.no_md_entries) {
    if (success == false)
      break;
    if (exchange_time_utc < item.md_entry_date)
      exchange_time_utc = item.md_entry_date;
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        success = mbp_update(
            _bid,
            bid_length,
            item);
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        success = mbp_update(
            _ask,
            ask_length,
            item);
        break;
      }
      case core::fix::MDEntryType::TRADE: {
        success = trade_update(
            _trade,
            trade_length,
            item);
        break;
      }
      case core::fix::MDEntryType::INDEX_VALUE:
      case core::fix::MDEntryType::SETTLEMENT_PRICE:
        // FIXME(thraneh): how to propagate these???
        VLOG(4)(
            FMT_STRING("unsupported: {}"),
            item);
        break;
      default:
        LOG(WARNING)(
          FMT_STRING("unsupported: {}"),
          item);
        break;
    }
  }
  if (unlikely(success == false)) {
    LOG(FATAL)(
        FMT_STRING(
          "Insufficient bid/ask/trade array size(s): "
          "len(bid)={}/{}, len(ask)={}/{}, len(trade)={}/{}"),
        bid_length, _bid.size(),
        ask_length, _ask.size(),
        trade_length, _trade.size());
  }
  if (bid_length > 0 || ask_length > 0) {
    MarketByPrice market_by_price {
      .exchange = FLAGS_exchange,
      .symbol = market_data_incremental_refresh.symbol,
      .bids = {
        .items = _bid.data(),
        .length = bid_length,
      },
      .asks = {
        .items = _ask.data(),
        .length = ask_length,
      },
      .snapshot = false,  // incremental
      .exchange_time_utc = exchange_time_utc,
    };
    auto last = trade_length == 0;
    enqueue(
        market_by_price,
        last);
  }
  if (trade_length > 0) {
    TradeSummary trade_summary {
      .exchange = FLAGS_exchange,
      .symbol = market_data_incremental_refresh.symbol,
      .trades = {
        .items = _trade.data(),
        .length = trade_length,
      },
      .exchange_time_utc = exchange_time_utc,
    };
    enqueue(
        trade_summary,
        true);
  }
}

void Gateway::operator()(
    const fix::MarketDataRequestReject&) {
  assert(_gateway_status == GatewayStatus::READY);
  LOG(FATAL)("Unexpected");  // don't know how to continue
}

void Gateway::operator()(
    const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
  assert(_gateway_status == GatewayStatus::READY);
  LOG(INFO)(
      FMT_STRING("Received market data snapshot for symbol=\"{}\""),
      market_data_snapshot_full_refresh.symbol);
  size_t bid_length = 0, ask_length = 0;
  for (auto& item : market_data_snapshot_full_refresh.no_md_entries) {
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        mbp_update(_bid, bid_length, item);
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        mbp_update(_ask, ask_length, item);
        break;
      }
      default:
        break;
    }
  }
  MarketByPrice market_by_price {
    .exchange = FLAGS_exchange,
    .symbol = market_data_snapshot_full_refresh.symbol,
    .bids = {
      .items = _bid.data(),
      .length = bid_length,
    },
    .asks = {
      .items = _ask.data(),
      .length = ask_length,
    },
    .snapshot = true,  // reset
    .exchange_time_utc = {},
  };
  enqueue(
      market_by_price,
      true);
}

void Gateway::operator()(
    const fix::OrderCancelReject& order_cancel_reject) {
  assert(_gateway_status == GatewayStatus::READY);

  server::OMS_Lookup order_lookup {
    .symbol = std::string_view(),
    .side = Side::UNDEFINED,
    .status = core::fix::map(order_cancel_reject.ord_status),
    .price = std::numeric_limits<double>::quiet_NaN(),
    .remaining_quantity = std::numeric_limits<double>::quiet_NaN(),
    .traded_quantity = std::numeric_limits<double>::quiet_NaN(),
    .commissions = std::numeric_limits<double>::quiet_NaN(),
    .timestamp = {},
    .external_order_id = std::string_view(),
  };

  auto found = _dispatcher.find_order(
      order_cancel_reject.cl_ord_id,
      order_cancel_reject.orig_cl_ord_id,
      order_lookup,
      [&](const auto& order, auto& result) {
    DLOG_IF(FATAL, order.request_type != RequestType::MODIFY_ORDER)("UNEXPECTED");

    result.origin = Origin::EXCHANGE;
    result.request_status = RequestStatus::REJECTED;
    result.error = fix::map_error(order_cancel_reject.text);
    result.text = order_cancel_reject.text;
  });

  LOG_IF(WARNING, found == false)("*** EXTERNAL ORDER ***");
}

void Gateway::operator()(
    const fix::PositionReport& position_report) {
  switch (position_report.pos_req_result) {
    case core::fix::PosReqResult::VALID:
      switch (position_report.pos_req_type) {
        case core::fix::PosReqType::POSITIONS: {
          for (auto& position : position_report.no_positions) {
            PositionUpdate buy {
              .account = _account,
              .exchange = FLAGS_exchange,
              .symbol = position.symbol,
              .side = Side::BUY,
              .position = position.long_qty,
              .last_trade_id = 0,
              .position_cost = 0.0,
              .position_yesterday = 0.0,
              .position_cost_yesterday = 0.0,
            };
            PositionUpdate sell {
              .account = _account,
              .exchange = FLAGS_exchange,
              .symbol = position.symbol,
              .side = Side::SELL,
              .position = position.short_qty,
              .last_trade_id = 0,
              .position_cost = 0.0,
              .position_yesterday = 0.0,
              .position_cost_yesterday = 0.0,
            };
            enqueue(
                buy,
                false);
            enqueue(
                sell,
                true);
          }
          break;
        }
        default:
          DLOG(FATAL)("UNEXPECTED");
          break;
      }
      break;
    default:
      DLOG(FATAL)("UNEXPECTED");
      break;
  }
  check_download(Download::POSITIONS);
}

void Gateway::operator()(
    const fix::Reject& reject) {
  LOG(WARNING)(
      FMT_STRING("reject={}"),
      reject);
  assert(_gateway_status != GatewayStatus::DISCONNECTED);

  LOG(FATAL)("Not supported");
}

void Gateway::operator()(
    const fix::SecurityList& security_list) {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  _currencies.clear();
  if (security_list.no_related_sym.size() > 0) {
    assert(_symbols.empty());
    size_t security_count = 0;
    _symbols.reserve(security_list.no_related_sym.size());  // note! alloc
    for (auto& instrument : security_list.no_related_sym) {
      // note!
      //   USD will cause a Reject
      //   using commission currency because it requires funding
      if (instrument.comm_currency.empty() == false)
        _currencies.emplace(instrument.comm_currency);
      if (_dispatcher.discard_symbol(instrument.symbol))
        continue;
      _symbols.emplace_back(instrument.symbol);
      ReferenceData reference_data {
        .exchange = FLAGS_exchange,
        .symbol = instrument.symbol,
        .security_type = fix::map_security_type(instrument.security_type),
        .currency = instrument.currency,
        .settlement_currency = instrument.settl_currency,
        .commission_currency = instrument.comm_currency,
        .tick_size = instrument.min_price_increment,
        .limit_up = std::numeric_limits<double>::quiet_NaN(),
        .limit_down = std::numeric_limits<double>::quiet_NaN(),
        .multiplier = instrument.contract_multiplier,
        .min_trade_vol = instrument.min_trade_vol,
        .option_type = core::fix::map(instrument.put_or_call),
        .strike_currency = instrument.strike_currency,
        .strike_price = instrument.strike_price,
      };
      enqueue(
          reference_data,
          true);
      ++security_count;
    }
    VLOG(1)(
        FMT_STRING("- securities: {} (/{})"),
        security_count,
        security_list.no_related_sym.size());
  }
  check_download(Download::SECURITIES);
}

void Gateway::operator()(
    const fix::UserResponse& user_response) {
  FundsUpdate funds_update {
    .account = _account,
    .currency = user_response.currency,
    .balance = user_response.deribit_user_balance,
    .hold = double{0.0},
  };
  enqueue(
      funds_update,
      true);
  if (_download == Download::USER) {
    if (_download_users && 0 == --_download_users)
      check_download(Download::USER);
  }
}

void Gateway::update(GatewayStatus gateway_status) {
  if (gateway_status == _gateway_status)
    return;
  _gateway_status = gateway_status;
  MarketDataStatus market_data_status {
    .status = _gateway_status,
  };
  enqueue(
      market_data_status,
      false);
  OrderManagerStatus order_manager_status {
    .account = _account,
    .status = _gateway_status,
  };
  enqueue(
      order_manager_status,
      true);
  LOG(INFO)(
      FMT_STRING("Update: gateway_status={}"),
      _gateway_status);
}

void Gateway::begin_download() {
  assert(_download == Download::NONE);
  update(GatewayStatus::DOWNLOADING);
  LOG(INFO)("Download:");
  download_securities();
}

void Gateway::check_download(Download download) {
  if (download != _download)
    return;
  switch (_download) {
    case Download::NONE:
      return;
    case Download::SECURITIES:
      LOG(INFO)("Download securities COMPLETED");
      download_positions();
      break;
    case Download::POSITIONS:
      LOG(INFO)("Download positions COMPLETED");
      download_orders();
      break;
    case Download::ORDERS:
      LOG(INFO)("Download orders COMPLETED");
      download_user();
      break;
    case Download::USER: {
      LOG(INFO)("Download user COMPLETED");
      update(GatewayStatus::READY);
      LOG(INFO)("Download COMPLETED");
      _download = Download::NONE;
      _download_timestamp = {};
      subscribe_market_data();
      server::PRINT_REDUCED_LOGGING();
      break;
    };
  }
}

void Gateway::download_securities() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("... download securities");
  auto request_id = _dispatcher.next_request_id();
  fix::SecurityListRequest security_list_request {
    .security_req_id = request_id,
    .security_list_request_type =
      core::fix::SecurityListRequestType::ALL_SECURITIES,
  };
  _fix(security_list_request);
  _download = Download::SECURITIES;
  _download_timestamp = core::get_system_clock();
}

void Gateway::download_positions() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("... download positions");
  auto request_id = _dispatcher.next_request_id();
  fix::RequestForPositions request_for_positions {
    .pos_req_id = request_id,
    .pos_req_type = core::fix::PosReqType::POSITIONS,
    .subscription_request_type =
      roq::core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
    .currency = std::string_view(),
  };
  _fix(request_for_positions);
  _download = Download::POSITIONS;
  _download_timestamp = core::get_system_clock();
}

void Gateway::download_orders() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("... download orders");
  auto request_id = _dispatcher.next_request_id();
  fix::OrderMassStatusRequest order_mass_status_request {
    .mass_status_req_id = request_id,
    .mass_status_req_type = core::fix::MassStatusReqType::ORDERS,
  };
  _fix(order_mass_status_request);
  _download = Download::ORDERS;
  _download_timestamp = core::get_system_clock();
}

void Gateway::download_user() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("... download user");
  assert(_currencies.empty() == false);
  _download_users = _currencies.size();  // async countdown
  for (auto& currency : _currencies) {
    auto request_id = _dispatcher.next_request_id();
    fix::UserRequest user_request_btc {
      .user_request_id = request_id,
      .user_request_type = core::fix::UserRequestType::REQUEST_INDIVIDUAL_USER_STATUS,
      .username = _access_key,
      .currency = static_cast<std::string_view>(currency),
    };
    _fix(user_request_btc);
  }
  _download = Download::USER;
  _download_timestamp = core::get_system_clock();
}

void Gateway::subscribe_market_data() {
  assert(_gateway_status == GatewayStatus::READY);
  if (_symbols.empty()) {
    LOG(WARNING)("Can't subscribe market data, reason: NO SYMBOLS");
    return;
  }
  LOG(INFO)("Subscribe market data");
  fix::MDReq md_entry_types[] = { {
      .md_entry_type = core::fix::MDEntryType::BID,
    }, {
      .md_entry_type = core::fix::MDEntryType::OFFER,
    }, {
      .md_entry_type = core::fix::MDEntryType::TRADE,
    }
  };
  std::vector<fix::InstrmtMDReq> related_sym(FLAGS_max_batch_size);
  for (size_t i = 0;; ++i) {
    auto offset = i * FLAGS_max_batch_size;
    if (_symbols.size() < offset)
      break;
    auto count = std::min<size_t>(
        _symbols.size() - offset,
        FLAGS_max_batch_size);
    if (count) {
      for (size_t i = 0; i < count; ++i)
        related_sym[i].symbol = _symbols[offset + i];
      auto request_id = _dispatcher.next_request_id();
      fix::MarketDataRequest market_data_request {
        .md_req_id = request_id,
        .subscription_request_type = core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
        .market_depth = 20,  // the maximum
        .md_update_type = core::fix::MDUpdateType::INCREMENTAL_REFRESH,
        .deribit_trade_amount = 0,  // none
        .deribit_since_timestamp = {},  // none
        .no_md_entry_types = roq::span(
            md_entry_types,
            std::size(md_entry_types)),
        .no_related_sym = roq::span(
            related_sym.data(),
            count),
      };
      _fix(market_data_request);
    }
  }
}

void Gateway::reset() {
  _download = Download::NONE;
  _symbols.clear();
}

template <typename T>
inline void Gateway::enqueue(
    const T& value,
    bool is_last) {
  auto now = core::get_system_clock();
  _dispatcher(
      value,
      now,
      now,
      is_last);
}

template <typename T>
inline void Gateway::enqueue(
    uint8_t user_id,
    const T& value,
    bool is_last) {
  auto now = core::get_system_clock();
  _dispatcher(
      user_id,
      value,
      now,
      now,
      is_last);
}

}  // namespace deribit
}  // namespace roq
