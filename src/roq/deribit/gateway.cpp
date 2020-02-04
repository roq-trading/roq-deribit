/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <limits>
#include <utility>

#include "roq/core/fix/utils.h"

#include "roq/deribit/options.h"

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
  new (&data[offset++]) MBPUpdate {
    .price = item.md_entry_px,
    .quantity = item.md_entry_size,
  };
  return offset < data.size();
}

template <typename T>
static bool trade_update(
    auto& data,
    size_t& offset,
    const T& item) {
  auto& trade = data[offset++];
  new (&trade) Trade {
    .side = core::fix::map(item.side),
    .price = item.md_entry_px,
    .quantity = item.md_entry_size,
    .trade_id = {},  // copy string (following statement)
  };
  item.deribit_trade_id.copy(
      trade.trade_id,
      sizeof(trade.trade_id));
  return offset < data.size();
}

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const Config& config)
    : _dispatcher(dispatcher),
      _account(config.get_account()),
      _access_key(config.get_access_key()),
      _dns_base(_base, true),
      _fix(
          *this,
          config,
          _base,
          _dns_base),
      _bid(FLAGS_max_depth),
      _ask(FLAGS_max_depth),
      _trade(FLAGS_max_trades) {
  LOG_IF(WARNING, FLAGS_cancel_on_disconnect == false)(
      "Orders will *NOT* be cancelled on disconnect");
}

void Gateway::operator()(const StartEvent& event) {
  LOG(INFO)("Starting the gateway...");
  _fix(event);
}

void Gateway::operator()(const StopEvent& event) {
  LOG(INFO)("Stopping the gateway...");
  _fix(event);
}

void Gateway::operator()(const TimerEvent& event) {
  _fix(event);
  _base.loop(EVLOOP_NONBLOCK);
}

void Gateway::operator()(const ConnectionStatusEvent&) {
}

void Gateway::operator()(const CreateOrderEvent& event) {
  if (unlikely(validate(event) == false))
    return;
  auto& message_info = event.message_info;
  auto& create_order = event.create_order;
  // TODO(thraneh): check against max_order_id before continuing
  auto gateway_order_id = _dispatcher.next_order_id();
  OrderMapping order_mapping(
      message_info,
      create_order,
      gateway_order_id);
  auto key = order_mapping.key();
  if (unlikely(_order_mapping.find(key) != _order_mapping.end())) {
    _dispatcher.send_order_ack(
        event,
        Error::INVALID_ORDER_ID);
    return;
  } else {
    auto iter = _order_mapping.emplace(
        key,
        std::move(order_mapping)).first;
    // XXX fix this... replacing previous variable
    auto& order_mapping = (*iter).second;
    auto request_id = _fix.next_request_id();
    _order_lookup.emplace(request_id, key);
    fix::NewOrderSingle new_order_single {
      .cl_ord_id = request_id,
      .side = core::fix::map(create_order.side),
      .order_qty = create_order.quantity,
      .price = create_order.price,
      .symbol = create_order.symbol,
      .ord_type = core::fix::map(create_order.order_type),
      .time_in_force = core::fix::map(create_order.time_in_force),
      .deribit_label = (*iter).second.user_custom(),
    };
    try {
      _fix(new_order_single);
      order_mapping.update_request(
          request_id,
          RequestType::CREATE_ORDER);
      _dispatcher.send_order_ack(
          event,
          gateway_order_id,
          request_id);
    } catch (std::exception& e) {
      LOG(WARNING)(
          FMT_STRING("Exception: what=\"{}\""),
          e.what());
      _dispatcher.send_order_ack(
          event,
          Error::NETWORK_ERROR);
    }
  }
}

void Gateway::operator()(const ModifyOrderEvent& event) {
  auto& message_info = event.message_info;
  auto& modify_order = event.modify_order;
  auto key = OrderMapping::key(
      message_info.source,
      modify_order.order_id);
  auto iter = _order_mapping.find(key);
  if (unlikely(iter == _order_mapping.end())) {
    _dispatcher.send_order_ack(
        event,
        Error::UNKNOWN_ORDER_ID);
    return;
  }
  auto& order_mapping = (*iter).second;
  auto gateway_order_id = order_mapping.gateway_order_id();
  auto external_order_id = order_mapping.exchange_order_id();
  if (unlikely(validate(event, gateway_order_id, external_order_id) == false)) {
    return;
  }
  if (unlikely(order_mapping.ready() == false)) {
    _dispatcher.send_order_ack(
        event,
        Error::UNKNOWN_EXCHANGE_ORDER_ID,
        gateway_order_id,
        external_order_id);
    return;
  }
  auto request_id = _fix.next_request_id();
  fix::OrderCancelReplaceRequest order_cancel_replace_request {
    .cl_ord_id = request_id,
    .orig_cl_ord_id = order_mapping.exchange_order_id(),
    .side = core::fix::map(order_mapping.side()),
    .order_qty = modify_order.quantity,
    .ord_type = core::fix::map(order_mapping.order_type()),
    .price = modify_order.price,
    .symbol = order_mapping.symbol(),
    .transact_time = order_mapping.update_time(),
  };
  try {
    _fix(order_cancel_replace_request);
    order_mapping.update_request(
        request_id,
        RequestType::MODIFY_ORDER);
    _dispatcher.send_order_ack(
        event,
        gateway_order_id,
        external_order_id,
        request_id);
  } catch (std::exception& e) {
    LOG(WARNING)(
        FMT_STRING("Exception: what=\"{}\""),
        e.what());
    _dispatcher.send_order_ack(
        event,
        Error::NETWORK_ERROR,
        gateway_order_id,
        external_order_id);
  }
}

void Gateway::operator()(const CancelOrderEvent& event) {
  auto& message_info = event.message_info;
  auto& cancel_order = event.cancel_order;
  auto key = OrderMapping::key(
      message_info.source,
      cancel_order.order_id);
  auto iter = _order_mapping.find(key);
  if (unlikely(iter == _order_mapping.end())) {
    _dispatcher.send_order_ack(
        event,
        Error::UNKNOWN_ORDER_ID);
    return;
  }
  auto& order_mapping = (*iter).second;
  auto gateway_order_id = order_mapping.gateway_order_id();
  auto external_order_id = order_mapping.exchange_order_id();
  if (unlikely(validate(event, gateway_order_id, external_order_id) == false)) {
    return;
  }
  if (unlikely(order_mapping.ready() == false)) {
    _dispatcher.send_order_ack(
        event,
        Error::UNKNOWN_EXCHANGE_ORDER_ID,
        gateway_order_id,
        external_order_id);
    return;
  }
  auto request_id = _fix.next_request_id();
  fix::OrderCancelRequest order_cancel_request {
    .cl_ord_id = request_id,
    .orig_cl_ord_id = order_mapping.exchange_order_id(),
  };
  try {
    _fix(order_cancel_request);
    order_mapping.update_request(
        request_id,
        RequestType::CANCEL_ORDER);
    _dispatcher.send_order_ack(
        event,
        gateway_order_id,
        external_order_id,
        request_id);
  } catch (std::exception& e) {
    LOG(WARNING)(
        FMT_STRING("Exception: what=\"{}\""),
        e.what());
    _dispatcher.send_order_ack(
        event,
        Error::NETWORK_ERROR,
        gateway_order_id,
        external_order_id);
  }
}

void Gateway::operator()(Metrics& metrics) {
  _fix(metrics);
}

// fix

void Gateway::operator()(const FIX& fix) {
  if (fix.ready()) {
    switch (_gateway_status) {
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

void Gateway::operator()(
    const fix::ExecutionReport& execution_report) {
  // download begin?
  switch (execution_report.mass_status_req_type) {
    case core::fix::MassStatusReqType::ORDERS:
      assert(_gateway_status == GatewayStatus::DOWNLOADING);
      _download_execution_reports = execution_report.tot_num_reports;
      if (_download_execution_reports == 0)
        check_download();
      return;
    default:
      break;
  }

  // order mapping
  auto iter = find_order_mapping(
      execution_report.cl_ord_id,
      execution_report.orig_cl_ord_id);
  if (unlikely(iter == _order_mapping.end())) {
    if (execution_report.deribit_label.empty()) {
      LOG(WARNING)("*** EXTERNAL ORDER ***");
    } else {
      iter = create_order_mapping(execution_report);
    }
  }
  if (iter != _order_mapping.end()) {
    auto& order_mapping = (*iter).second;

    // FIXME(thraneh): DEBUG
    if (unlikely(
          order_mapping.validate(execution_report) == false)) {
      LOG(WARNING)("*** SOMETHING WRONG ***");
    } else {
      constexpr auto origin = Origin::EXCHANGE;
      auto status = RequestStatus::UNDEFINED;
      auto error = fix::map_error(execution_report.text);
      bool order_update = true;
      switch (execution_report.exec_type) {
        case core::fix::ExecType::REJECTED: {
          switch (order_mapping.request()) {
            case RequestType::UNDEFINED:
              LOG(WARNING)("*** EXTERNAL ACTION ***");
              break;
            case RequestType::CREATE_ORDER:
              order_update = false;
              [[ fallthrough ]];
            case RequestType::MODIFY_ORDER:
            case RequestType::CANCEL_ORDER:
              status = RequestStatus::REJECTED;
              break;
          }
          break;
        }
        case core::fix::ExecType::CANCELED: {
          switch (order_mapping.request()) {
            case RequestType::UNDEFINED:
              LOG(WARNING)("*** EXTERNAL ACTION ***");
              break;
            case RequestType::CREATE_ORDER:
            case RequestType::MODIFY_ORDER:
              LOG(FATAL)("DEBUG: UNEXPECTED");
              break;
            case RequestType::CANCEL_ORDER:
              status = RequestStatus::ACCEPTED;
              order_update = false;
              break;
          }
          break;
        }
        case core::fix::ExecType::ORDER_STATUS:
          switch (execution_report.ord_status) {
            case core::fix::OrdStatus::NEW: {
              switch (order_mapping.request()) {
                case RequestType::UNDEFINED:
                  switch (_download) {
                    case Download::NONE:
                      LOG(WARNING)("*** EXTERNAL ACTION ***");
                      break;
                    case Download::ORDERS:
                      break;
                    default:
                      LOG(FATAL)("DEBUG: UNEXPECTED");
                  }
                  break;
                case RequestType::CREATE_ORDER:
                case RequestType::MODIFY_ORDER:
                  status = RequestStatus::ACCEPTED;
                  break;
                case RequestType::CANCEL_ORDER:
                  LOG(FATAL)("DEBUG: UNEXPECTED");
                  break;
              }
              break;
            }
            case core::fix::OrdStatus::PARTIALLY_FILLED:
            case core::fix::OrdStatus::FILLED:
              break;
            case core::fix::OrdStatus::CANCELED:
              // TODO(thraneh): how to signal external action
              // -- we need to add an "expectation" into order_mapping?
              break;
            default:
              LOG(FATAL)("DEBUG: UNEXPECTED");
              break;
          }
          break;
        default:
          LOG(FATAL)("DEBUG: UNEXPECTED");
          break;
      }
      if (status != RequestStatus::UNDEFINED) {
        OrderAck order_ack {
          .account = _account,
          .order_id = order_mapping.user_order_id(),
          .type = order_mapping.request(),
          .origin = origin,
          .status = (error == Error::NONE)
            ? RequestStatus::ACCEPTED
            : RequestStatus::REJECTED,
          .error = error,
          .text = execution_report.text,
          .gateway_order_id = order_mapping.gateway_order_id(),
          .external_order_id = order_mapping.exchange_order_id(),
          .request_id = execution_report.orig_cl_ord_id,
        };
        enqueue(
            order_mapping.user_id(),
            order_ack,
            order_update == false);  // only "last" if no order_update
        order_mapping.reset_request();
      }
      if (order_update) {
        for (auto& fills : execution_report.fills_grp) {
          // TODO(thraneh): map fill_exec_id <-> local_trade_id ???
          auto trade_id = _dispatcher.next_trade_id();
          TradeUpdate trade_update {
            .account = _account,
            .trade_id = trade_id,
            .order_id = order_mapping.user_order_id(),
            .exchange = FLAGS_exchange,
            .symbol = execution_report.symbol,
            .side = order_mapping.side(),
            .quantity = fills.fill_qty,
            .price = fills.fill_px,
            .position_effect = PositionEffect::UNDEFINED,
            .order_template = std::string(),
            .create_time_utc = execution_report.transact_time,
            .update_time_utc = execution_report.transact_time,
            .gateway_order_id = order_mapping.gateway_order_id(),
            .gateway_trade_id = trade_id,
            .external_order_id = order_mapping.exchange_order_id(),
            .external_trade_id = fills.fill_exec_id,
          };
          enqueue(
              order_mapping.user_id(),
              trade_update,
              false);
        }
        OrderUpdate order_update {
          .account = _account,
          .order_id = order_mapping.user_order_id(),
          .exchange = FLAGS_exchange,
          .symbol = execution_report.symbol,
          .status = core::fix::map(execution_report.ord_status),
          .side = order_mapping.side(),
          .price = execution_report.price,
          .remaining_quantity = execution_report.leaves_qty,
          .traded_quantity = execution_report.cum_qty,
          .position_effect = PositionEffect::UNDEFINED,
          .order_template = std::string(),
          .create_time_utc = order_mapping.create_time(),
          .update_time_utc = order_mapping.update_time(),
          .commissions = execution_report.commission,
          .gateway_order_id = order_mapping.gateway_order_id(),
          .external_order_id = order_mapping.exchange_order_id(),
        };
        enqueue(
            order_mapping.user_id(),
            order_update,
            true);

      } else {
        LOG_IF(FATAL, execution_report.fills_grp.size() > 0)(
            "DEBUG: UNEXPECTED");
      }
    }

  } else {
    // TODO(thraneh): process fills? --> maintain positions
  }

  // download end?
  if (_download_execution_reports &&
      0 == --_download_execution_reports) {
    check_download();
  }
}

void Gateway::operator()(
    const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
  assert(_gateway_status == GatewayStatus::READY);
  bool success = true;
  size_t bid_length = 0, ask_length = 0, trade_length = 0;
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto& item : market_data_incremental_refresh.md_inc_grp) {
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
      FMT_STRING("Market data snapshot symbol=\"{}\""),
      market_data_snapshot_full_refresh.symbol);
  size_t bid_length = 0, ask_length = 0;
  for (auto& item : market_data_snapshot_full_refresh.md_full_grp) {
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
  auto iter = find_order_mapping(
      order_cancel_reject.cl_ord_id,
      order_cancel_reject.orig_cl_ord_id);
  if (iter == _order_mapping.end()) {
    LOG(WARNING)("*** EXTERNAL ORDER ***");
  } else {
    auto& order_mapping = (*iter).second;
    constexpr auto origin = Origin::EXCHANGE;
    auto status = RequestStatus::UNDEFINED;
    auto error = fix::map_error(order_cancel_reject.text);
    switch (order_mapping.request()) {
      case RequestType::UNDEFINED:
        LOG(WARNING)("*** EXTERNAL ACTION ***");
        break;
      case RequestType::CREATE_ORDER:
      case RequestType::MODIFY_ORDER:
        LOG(FATAL)("DEBUG: UNEXPECTED");
        break;
      case RequestType::CANCEL_ORDER:
        status = RequestStatus::REJECTED;
        break;
    }
    if (status != RequestStatus::UNDEFINED) {
      OrderAck order_ack {
        .account = _account,
        .order_id = order_mapping.user_order_id(),
        .type = order_mapping.request(),
        .origin = origin,
        .status = status,
        .error = error,
        .text = order_cancel_reject.text,
        .gateway_order_id = order_mapping.gateway_order_id(),
        .external_order_id = order_mapping.exchange_order_id(),
        .request_id = order_cancel_reject.orig_cl_ord_id,
      };
      enqueue(
          order_mapping.user_id(),
          order_ack,
          true);
      order_mapping.reset_request();
    }
  }
}

void Gateway::operator()(
    const fix::PositionReport& position_report) {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  switch (position_report.pos_req_result) {
    case core::fix::PosReqResult::VALID:
      switch (position_report.pos_req_type) {
        case core::fix::PosReqType::POSITIONS: {
          size_t position_count = 0;
          for (auto& position : position_report.positions) {
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
            ++position_count;
          }
          VLOG(1)(
              FMT_STRING("- positions: {} (/{})"),
              position_count,
              position_report.positions.size());
          break;
        }
        default:
          LOG(FATAL)("DEBUG: UNEXPECTED");
          break;
      }
      break;
    default:
      LOG(FATAL)("DEBUG: UNEXPECTED");
      break;
  }

  check_download();
}

void Gateway::operator()(
    const fix::Reject& reject) {
  assert(_gateway_status != GatewayStatus::DISCONNECTED);

  auto request_id = fmt::format(  // FIXME(thraneh): this is *wrong*
      FMT_STRING("roq:{:06}"),
      reject.ref_seq_num);
  // FIXME(thraneh): this lookup doesn't *just* manage cl_ord_id
  auto iter = find_order_mapping(request_id, std::string_view());
  if (iter != _order_mapping.end())  {
    auto& order_mapping = (*iter).second;
    auto error = fix::map_error(reject.text);
    OrderAck order_ack {
      .account = _account,
      .order_id = order_mapping.user_order_id(),
      .type = RequestType::CREATE_ORDER,  // FIXME(thraneh): from order_mapping
      .origin = Origin::EXCHANGE,
      .status = RequestStatus::REJECTED,
      .error = error,
      .text = reject.text,
      .gateway_order_id = order_mapping.gateway_order_id(),
      .external_order_id = order_mapping.exchange_order_id(),
      .request_id = request_id,
    };
    enqueue(
        order_mapping.user_id(),
        order_ack,
        true);
  } else {
    LOG(FATAL)("unexpected");  // XXX disconnect and restart ???
  }
}

void Gateway::operator()(
    const fix::SecurityList& security_list) {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  _currencies.clear();
  if (security_list.instruments.size() > 0) {
    assert(_symbols.empty());
    size_t security_count = 0;
    _symbols.reserve(security_list.instruments.size());  // note! alloc
    for (auto& instrument : security_list.instruments) {
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
          false);
      // note! we receive no information about the trading status
      MarketStatus market_status {
        .exchange = FLAGS_exchange,
        .symbol = instrument.symbol,
        .trading_status = TradingStatus::OPEN,  // TODO(thraneh): missing
      };
      enqueue(
          market_status,
          true);
      ++security_count;
    }
    VLOG(1)(
        FMT_STRING("- securities: {} (/{})"),
        security_count,
        security_list.instruments.size());
  }
  check_download();
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
      check_download();
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
  // XXX assert(_gateway_status == GatewayStatus::LOGIN_SENT);
  update(GatewayStatus::DOWNLOADING);
  LOG(INFO)("Download:");
  download_securities();
}

void Gateway::check_download() {
  assert(_download != Download::NONE);
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  switch (_download) {
    case Download::NONE:
      assert(false);
      break;
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
      subscribe_market_data();
      break;
    };
  }
}

void Gateway::download_securities() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("... download securities");
  auto request_id = _fix.next_request_id();
  fix::SecurityListRequest security_list_request {
    .security_req_id = request_id,
  };
  _fix(security_list_request);
  _download = Download::SECURITIES;
}

void Gateway::download_positions() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("... download positions");
  auto request_id = _fix.next_request_id();
  fix::RequestForPositions request_for_positions {
    .pos_req_id = request_id,
    .pos_req_type = core::fix::PosReqType::POSITIONS,
  };
  _fix(request_for_positions);
  _download = Download::POSITIONS;
}

void Gateway::download_orders() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("... download orders");
  auto request_id = _fix.next_request_id();
  fix::OrderMassStatusRequest order_mass_status_request {
    .mass_status_req_id = request_id,
    .mass_status_req_type = core::fix::MassStatusReqType::ORDERS,
  };
  _fix(order_mass_status_request);
  _download = Download::ORDERS;
}

void Gateway::download_user() {
  assert(_gateway_status == GatewayStatus::DOWNLOADING);
  LOG(INFO)("... download user");
  assert(_currencies.empty() == false);
  _download_users = _currencies.size();  // async countdown
  for (auto& currency : _currencies) {
    auto request_id = _fix.next_request_id();
    fix::UserRequest user_request_btc {
      .user_request_id = request_id,
      .username = _access_key,
      .currency = static_cast<std::string_view>(currency),
    };
    _fix(user_request_btc);
  }
  _download = Download::USER;
}

void Gateway::subscribe_market_data() {
  assert(_gateway_status == GatewayStatus::READY);
  if (_symbols.empty()) {
    LOG(WARNING)("Can't subscribe market data, reason: NO SYMBOLS");
    return;
  }
  LOG(INFO)("Subscribe market data");
  if (FLAGS_batch_subscribe)
    subscribe_market_data_batch();
  else
    subscribe_market_data_simple();
}

void Gateway::subscribe_market_data_batch() {
  // FIXME(thraneh): simplify this
  std::vector<std::string_view> symbols(FLAGS_max_batch_size);
  size_t j = 0;
  for (size_t i = 0; i < _symbols.size(); ++i) {
    symbols[j++] = _symbols[i];
    if (j == symbols.size()) {
      auto request_id = _fix.next_request_id();
      fix::MarketDataRequest market_data_request {
        .md_req_id = request_id,
        .symbol = {},
        .symbols = symbols,
      };
      _fix(market_data_request);
      j = 0;
    }
  }
  if (j > 0) {
    auto request_id = _fix.next_request_id();
    fix::MarketDataRequest market_data_request {
      .md_req_id = request_id,
      .symbol = {},
      .symbols = symbols,
    };
    _fix(market_data_request);
  }
}

void Gateway::subscribe_market_data_simple() {
  for (auto& symbol : _symbols) {
    auto request_id = _fix.next_request_id();
    fix::MarketDataRequest market_data_request {
      .md_req_id = request_id,
      .symbol = symbol,
      .symbols = {},
    };
    _fix(market_data_request);
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

bool Gateway::validate(const CreateOrderEvent& event) {
  auto& create_order = event.create_order;
  auto error = Error::NONE;
  if (unlikely(_gateway_status != GatewayStatus::READY)) {
    error = Error::GATEWAY_NOT_READY;
  } else if (unlikely(
        create_order.account.compare(_account) != 0)) {
    error = Error::INVALID_ACCOUNT;
  } else if (unlikely(
        create_order.exchange.compare(FLAGS_exchange) != 0)) {
    error = Error::INVALID_EXCHANGE;
  } else if (unlikely(
        create_order.position_effect != PositionEffect::UNDEFINED)) {
    error = Error::INVALID_POSITION_EFFECT;
  } else if (unlikely(
        create_order.order_template.empty() == false &&
        create_order.order_template.compare("default") != 0)) {
    error = Error::INVALID_ORDER_TEMPLATE;
  } else {
    return true;
  }
  assert(error != Error::NONE);
  _dispatcher.send_order_ack(
      event,
      error);
  return false;
}

bool Gateway::validate(
    const ModifyOrderEvent& event,
    uint32_t gateway_order_id,
    const std::string_view& external_order_id) {
  auto& modify_order = event.modify_order;
  auto error = Error::NONE;
  if (unlikely(_gateway_status != GatewayStatus::READY)) {
    error = Error::GATEWAY_NOT_READY;
  } else if (unlikely(modify_order.account.compare(_account) != 0)) {
    error = Error::INVALID_ACCOUNT;
  } else {
    return true;
  }
  assert(error != Error::NONE);
  _dispatcher.send_order_ack(
      event,
      error,
      gateway_order_id,
      external_order_id);
  return false;
}

bool Gateway::validate(
    const CancelOrderEvent& event,
    uint32_t gateway_order_id,
    const std::string_view& external_order_id) {
  auto& cancel_order = event.cancel_order;
  auto error = Error::NONE;
  if (unlikely(_gateway_status != GatewayStatus::READY)) {
    error = Error::GATEWAY_NOT_READY;
  } else if (unlikely(
        cancel_order.account.compare(_account) != 0)) {
    error = Error::INVALID_ACCOUNT;
  } else {
    return true;
  }
  assert(error != Error::NONE);
  _dispatcher.send_order_ack(
      event,
      error,
      gateway_order_id,
      external_order_id);
  return false;
}

decltype(Gateway::_order_mapping)::iterator
Gateway::find_order_mapping(
    const std::string_view& cl_ord_id,
    const std::string_view& orig_cl_ord_id) {
  auto iter = _order_lookup.find(cl_ord_id);
  if (unlikely(iter == _order_lookup.end())) {
    iter = _order_lookup.find(orig_cl_ord_id);
    if (unlikely(iter == _order_lookup.end())) {
      return _order_mapping.end();
    } else {
      // replace orig_cl_ord_id with cl_ord_id
      auto key = (*iter).second;
      _order_lookup.erase(iter);
      iter = _order_lookup.emplace(std::string(cl_ord_id), key).first;
    }
  }
  return _order_mapping.find((*iter).second);
}

decltype(Gateway::_order_mapping)::iterator
Gateway::create_order_mapping(
    const fix::ExecutionReport& execution_report) {
  try {
    OrderMapping order_mapping(
        execution_report,
        execution_report.deribit_label);
    auto key = order_mapping.key();
    auto iter = _order_mapping.emplace(
        key,
        std::move(order_mapping)).first;
    _order_lookup.emplace(
        std::string(execution_report.cl_ord_id),
        key);
    return iter;
  } catch (core::oms::InvalidUserCustom&) {
    LOG(WARNING)("*** INVALID USER_CUSTOM ***");
    return _order_mapping.end();
  }
}

}  // namespace deribit
}  // namespace roq
