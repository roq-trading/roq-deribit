/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <algorithm>
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
static bool mbp_update(auto &data, size_t &offset, const T &item) {
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
  auto &obj = data[offset];
  new (&obj) MBPUpdate{
      .price = item.md_entry_px,
      .quantity = item.md_entry_size,
  };
  ++offset;
  return offset < data.size();
}

template <typename T>
static bool trade_update(auto &data, size_t &offset, const T &item) {
  auto &obj = data[offset];
  new (&obj) Trade{
      .side = core::fix::map(item.side),
      .price = item.md_entry_px,
      .quantity = item.md_entry_size,
      .trade_id = item.deribit_trade_id,
  };
  ++offset;
  return offset < data.size();
}

template <typename T>
static bool fill_update(
    auto &dispatcher, auto &data, size_t &offset, const T &item) {
  auto trade_id = dispatcher.next_trade_id();
  auto &obj = data[offset];
  new (&obj) Fill{
      .quantity = item.fill_qty,
      .price = item.fill_px,
      .trade_id = trade_id,
      .gateway_trade_id = trade_id,
      .external_trade_id = item.fill_exec_id,
  };
  ++offset;
  return offset < data.size();
}

Gateway::Gateway(server::Dispatcher &dispatcher, const Config &config)
    : _dispatcher(dispatcher), _account(config.get_account()),
      _access_key(config.get_access_key()), _random(config.get_access_secret()),
      _dns_base(_base, true),
      _fix{
          .connection =
              {
                  *this,
                  config,
                  _random,
                  _base,
                  _dns_base,
              },
          .download = FIXDownload(
              std::chrono::seconds{FLAGS_download_timeout_secs},
              [this](auto state) { return download(state); }),
      },
      _web_socket{
          .connection =
              {*this, config, _random, _base, _dns_base, _ssl_context},
          .download = WebSocketDownload(
              std::chrono::seconds{FLAGS_download_timeout_secs},
              [this](auto state) { return download(state); }),
      },
      _fill(FLAGS_cache_fills_max_depth), _bid(FLAGS_cache_mbp_max_depth),
      _ask(FLAGS_cache_mbp_max_depth), _trade(FLAGS_cache_trades_max_depth),
      _statistics(StatisticsType::MAX) {
  LOG_IF(WARNING, FLAGS_fix_cancel_on_disconnect == false)
  ("Orders will *NOT* be cancelled on disconnect");
}

void Gateway::operator()(const Event<Start> &event) {
  LOG(INFO)("Starting the gateway...");
  _web_socket.connection(event);
  _fix.connection(event);
}

void Gateway::operator()(const Event<Stop> &event) {
  LOG(INFO)("Stopping the gateway...");
  _web_socket.connection(event);
  _fix.connection(event);
}

void Gateway::operator()(const Event<Timer> &event) {
  _fix.connection(event);
  _web_socket.connection(event);
  // fix
  /*
  if (_fix.download.has_expired()) {
    LOG(WARNING)("FIX download has timed out");
    _fix.download.reset();
    _fix.connection.close();
  }
  */
  // web socket
  /*
  if (_web_socket.download.has_expired()) {
    LOG(WARNING)("WebSocket download has timed out");
    _web_socket.download.reset();
    _web_socket.connection.close();
  }
  */
  _base.loop(EVLOOP_NONBLOCK);
}

void Gateway::operator()(const Event<Connection> &) {
}

void Gateway::operator()(
    const Event<CreateOrder> &event,
    const std::string_view &request_id,
    uint32_t gateway_order_id) {
  auto &message_info = event.message_info;
  auto &create_order = event.value;
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
  std::string_view deribit_label(buffer.data(), buffer.size());
  fix::NewOrderSingle new_order_single{
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
  _fix.connection(new_order_single);
}

void Gateway::operator()(
    const Event<ModifyOrder> &event,
    const std::string_view &request_id,
    const server::OMS_Order &order) {
  auto &modify_order = event.value;
  fix::OrderCancelReplaceRequest order_cancel_replace_request{
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
  _fix.connection(order_cancel_replace_request);
}

void Gateway::operator()(
    const Event<CancelOrder> &,
    const std::string_view &request_id,
    const server::OMS_Order &order) {
  fix::OrderCancelRequest order_cancel_request{
      .cl_ord_id = request_id,
      .orig_cl_ord_id = order.external_order_id,
  };
  _fix.connection(order_cancel_request);
}

void Gateway::operator()(metrics::Writer &writer) {
  _web_socket.connection(writer);
  _fix.connection(writer);
}

// web socket

uint32_t Gateway::download(WebSocketDownload::State state) {
  switch (state) {
    case WebSocketDownload::State::UNDEFINED:
      break;
    case WebSocketDownload::State::CURRENCIES:
      assert(_currencies_2.empty());
      _web_socket.connection.get_currencies();
      return 1;
    case WebSocketDownload::State::INSTRUMENTS:
      assert(_symbols_2.empty());
      for (auto &currency : _currencies_2)
        _web_socket.connection.get_instruments(currency);
      return _currencies_2.size();
    case WebSocketDownload::State::POSITIONS:
      for (auto &currency : _currencies_2)
        _web_socket.connection.get_positions(currency);
      return _currencies_2.size();
    case WebSocketDownload::State::DONE:
      LOG(INFO)("Ready");
      _web_socket.connection.subscribe_ticker(
          roq::span(_symbols_2.data(), _symbols_2.size()));
      return 0;
  }
  assert(false);
  return 0;
}

void Gateway::operator()(const WebSocket &) {
  if (_web_socket.connection.ready()) {
    _web_socket.download.begin();
  } else {
    _web_socket.download.reset();
    _currencies_2.clear();
    _symbols_2.clear();
  }
}

void Gateway::operator()(
    const json::Currencies &currencies, const server::TraceInfo &) {
  constexpr auto state = WebSocketDownload::State::CURRENCIES;
  VLOG(1)(R"(currencies={})", currencies);
  assert(_currencies_2.empty());
  std::transform(
      currencies.data.begin(),
      currencies.data.end(),
      std::back_inserter(_currencies_2),
      [](const auto &item) { return std::string(item.currency); });
  _web_socket.download.check(state);
}

void Gateway::operator()(
    const json::Instruments &instruments, const server::TraceInfo &) {
  constexpr auto state = WebSocketDownload::State::INSTRUMENTS;
  VLOG(1)(R"(instruments={})", instruments);
  for (auto &item : instruments.data) {
    if (_dispatcher.discard_symbol(item.instrument_name)) continue;
    _symbols_2.emplace_back(item.instrument_name);
  }
  _web_socket.download.check(state);
}

void Gateway::operator()(
    const json::Positions &positions, const server::TraceInfo &) {
  constexpr auto state = WebSocketDownload::State::POSITIONS;
  VLOG(1)(R"(positions={})", positions);
  // XXX do something
  _web_socket.download.check(state);
}

void Gateway::operator()(
    const json::Ticker &ticker, const server::TraceInfo &trace_info) {
  VLOG(3)(R"(ticker={})", ticker);
  TopOfBook top_of_book = {
      .exchange = FLAGS_exchange,
      .symbol = ticker.instrument_name,
      .layer =
          {
              .bid_price = ticker.best_bid_price,
              .bid_quantity = ticker.best_bid_amount,
              .ask_price = ticker.best_ask_price,
              .ask_quantity = ticker.best_ask_amount,
          },
      .snapshot = false,
      .exchange_time_utc = ticker.timestamp,
  };
  server::create_trace_and_dispatch(trace_info, top_of_book, _dispatcher, true);
  auto trading_status = json::map(ticker.state);
  auto &item = _trading_status[ticker.instrument_name];
  if (item != trading_status) {
    item = trading_status;
    MarketStatus market_status{
        .exchange = FLAGS_exchange,
        .symbol = ticker.instrument_name,
        .trading_status = json::map(ticker.state),
    };
    server::create_trace_and_dispatch(
        trace_info, market_status, _dispatcher, true);
  }
}

// fix

uint32_t Gateway::download(FIXDownload::State state) {
  switch (state) {
    case FIXDownload::State::UNDEFINED:
      assert(false);
      break;
    case FIXDownload::State::SECURITIES:
      download_securities();
      return 1;
    case FIXDownload::State::POSITIONS:
      download_positions();
      return 1;
    case FIXDownload::State::ORDERS:
      download_orders();
      return 1;  // first ExecutionReport has the real number
    case FIXDownload::State::USER:
      download_user();
      return _currencies.size();
    case FIXDownload::State::DONE:
      LOG(INFO)("Ready");
      update(GatewayStatus::READY);
      subscribe_market_data();
      return 0;
  }
  assert(false);
  return 0;
}

void Gateway::operator()(const FIX &) {
  if (_fix.connection.ready()) {
    update(GatewayStatus::DOWNLOADING);
    _fix.download.begin();
  } else {
    update(GatewayStatus::DISCONNECTED);
    _fix.download.reset();
    _symbols.clear();
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
    auto request_type, auto exec_type, auto ord_status, bool download) {
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
    const fix::ExecutionReport &execution_report,
    const server::TraceInfo &trace_info) {
  DLOG(INFO)(R"(execution_report={})", execution_report);

  // download begin?
  switch (execution_report.mass_status_req_type) {
    case core::fix::MassStatusReqType::ORDERS:
      _fix.download.update(
          FIXDownload::State::ORDERS, execution_report.tot_num_reports);
      return;
    default:
      break;
  }

  server::OMS_Lookup order_lookup{
      .symbol = execution_report.symbol,
      .side = core::fix::map(execution_report.side),
      .status = core::fix::map(execution_report.ord_status),
      .price = execution_report.price,
      .remaining_quantity = execution_report.leaves_qty,
      .traded_quantity = execution_report.cum_qty,
      .timestamp = execution_report.transact_time,
      .external_order_id = execution_report.order_id,
  };

  // XXX we used to also create orders here...
  auto found = _dispatcher.find_order(
      execution_report.cl_ord_id,
      execution_report.orig_cl_ord_id,
      order_lookup,
      trace_info,
      [&](const auto &order, auto &result) {
        result.request_status = compute_request_status(
            order.request_type,
            execution_report.exec_type,
            execution_report.ord_status,
            _fix.download.downloading(FIXDownload::State::ORDERS));

        if (result.request_status != RequestStatus::UNDEFINED) {
          result.origin = Origin::EXCHANGE;
          result.error = fix::map_error(execution_report.text);
          result.text = execution_report.text;
        }

        size_t fill_length = 0;
        bool success = true;
        for (auto &item : execution_report.no_fills) {
          if (success == false) break;
          success = fill_update(_dispatcher, _fill, fill_length, item);
        }
        if (ROQ_UNLIKELY(success == false)) {
          LOG(FATAL)
          (R"(Insufficient fill array size(s): )"
           R"(len(fill)={}/{}={}/{})",
           fill_length,
           execution_report.no_fills.size());
        }
        if (fill_length) {
          TradeUpdate trade_update{
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
              .fills = {_fill.data(), fill_length},
          };
          server::create_trace_and_dispatch(
              trace_info, trade_update, _dispatcher, true, order.user_id);
        }
      });

  // TODO(thraneh): process fills? --> maintain positions

  if (found == false) {
    auto external = execution_report.deribit_label.empty();
    if (external) {
      LOG(WARNING)("*** EXTERNAL ORDER ***");
    } else {
      LOG(WARNING)("*** UNKNOWN INTERNAL ORDER ***");
    }
    LOG(WARNING)("execution_report={}", execution_report);
  }

  // download end?
  _fix.download.check_relaxed(FIXDownload::State::ORDERS);
}

void Gateway::operator()(
    const fix::MarketDataIncrementalRefresh &market_data_incremental_refresh,
    const server::TraceInfo &trace_info) {
  assert(_gateway_status == GatewayStatus::READY);
  bool success = true;
  size_t bid_length = 0, ask_length = 0, trade_length = 0,
         statistics_length = 0;
  // open interest
  new (&_statistics[statistics_length++]) Statistics{
      .type = StatisticsType::PRE_OPEN_INTEREST,
      .value = market_data_incremental_refresh.open_interest,
  };
  // mark price
  new (&_statistics[statistics_length++]) Statistics{
      .type = StatisticsType::PRE_SETTLEMENT_PRICE,
      .value = market_data_incremental_refresh.mark_price,
  };
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : market_data_incremental_refresh.no_md_entries) {
    if (success == false) break;
    if (exchange_time_utc < item.md_entry_date)
      exchange_time_utc = item.md_entry_date;
    switch (item.md_entry_type) {
      case core::fix::MDEntryType::BID: {
        success = mbp_update(_bid, bid_length, item);
        break;
      }
      case core::fix::MDEntryType::OFFER: {
        success = mbp_update(_ask, ask_length, item);
        break;
      }
      case core::fix::MDEntryType::TRADE: {
        success = trade_update(_trade, trade_length, item);
        break;
      }
      case core::fix::MDEntryType::INDEX_VALUE:
        new (&_statistics[statistics_length++]) Statistics{
            .type = StatisticsType::INDEX_VALUE,
            .value = item.md_entry_px,
        };
        break;
      case core::fix::MDEntryType::SETTLEMENT_PRICE:
        new (&_statistics[statistics_length++]) Statistics{
            .type = StatisticsType::SETTLEMENT_PRICE,
            .value = item.md_entry_px,
        };
        break;
      default:
        LOG(WARNING)(R"(unsupported: {})", item);
        break;
    }
  }
  if (ROQ_UNLIKELY(success == false)) {
    LOG(FATAL)
    (R"(Insufficient bid/ask/trade array size(s): )"
     R"(len(bid)={}/{}, len(ask)={}/{}, len(trade)={}/{})",
     bid_length,
     _bid.size(),
     ask_length,
     _ask.size(),
     trade_length,
     _trade.size());
  }
  if (bid_length > 0 || ask_length > 0) {
    MarketByPriceUpdate market_by_price_update{
        .exchange = FLAGS_exchange,
        .symbol = market_data_incremental_refresh.symbol,
        .bids =
            {
                .items = _bid.data(),
                .length = bid_length,
            },
        .asks =
            {
                .items = _ask.data(),
                .length = ask_length,
            },
        .snapshot = false,  // incremental
        .exchange_time_utc = exchange_time_utc,
    };
    VLOG(3)("market_by_price_update={}", market_by_price_update);
    auto last = trade_length == 0;
    server::create_trace_and_dispatch(
        trace_info, market_by_price_update, _dispatcher, last);
  }
  if (trade_length > 0) {
    TradeSummary trade_summary{
        .exchange = FLAGS_exchange,
        .symbol = market_data_incremental_refresh.symbol,
        .trades =
            {
                .items = _trade.data(),
                .length = trade_length,
            },
        .exchange_time_utc = exchange_time_utc,
    };
    VLOG(3)("trade_summary={}", trade_summary);
    server::create_trace_and_dispatch(
        trace_info, trade_summary, _dispatcher, true);
  }
  if (statistics_length > 0) {
    StatisticsUpdate statistics_update{
        .exchange = FLAGS_exchange,
        .symbol = market_data_incremental_refresh.symbol,
        .statistics = roq::span(_statistics.data(), statistics_length),
        .snapshot = false,
        .exchange_time_utc = exchange_time_utc,
    };
    VLOG(3)("statistics_update={}", statistics_update);
    server::create_trace_and_dispatch(
        trace_info, statistics_update, _dispatcher, true);
  }
}

void Gateway::operator()(
    const fix::MarketDataRequestReject &, const server::TraceInfo &) {
  assert(_gateway_status == GatewayStatus::READY);
  LOG(FATAL)("Unexpected");  // don't know how to continue
}

void Gateway::operator()(
    const fix::MarketDataSnapshotFullRefresh &market_data_snapshot_full_refresh,
    const server::TraceInfo &trace_info) {
  assert(_gateway_status == GatewayStatus::READY);
  VLOG(1)
  (R"(Received market data snapshot for symbol="{}")",
   market_data_snapshot_full_refresh.symbol);
  size_t bid_length = 0, ask_length = 0;
  for (auto &item : market_data_snapshot_full_refresh.no_md_entries) {
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
  MarketByPriceUpdate market_by_price_update{
      .exchange = FLAGS_exchange,
      .symbol = market_data_snapshot_full_refresh.symbol,
      .bids =
          {
              .items = _bid.data(),
              .length = bid_length,
          },
      .asks =
          {
              .items = _ask.data(),
              .length = ask_length,
          },
      .snapshot = true,  // reset
      .exchange_time_utc = {},
  };
  server::create_trace_and_dispatch(
      trace_info, market_by_price_update, _dispatcher, true);
}

void Gateway::operator()(
    const fix::OrderCancelReject &order_cancel_reject,
    const server::TraceInfo &trace_info) {
  assert(_gateway_status == GatewayStatus::READY);

  server::OMS_Lookup order_lookup{
      .symbol = std::string_view(),
      .side = Side::UNDEFINED,
      .status = core::fix::map(order_cancel_reject.ord_status),
      .price = std::numeric_limits<double>::quiet_NaN(),
      .remaining_quantity = std::numeric_limits<double>::quiet_NaN(),
      .traded_quantity = std::numeric_limits<double>::quiet_NaN(),
      .timestamp = {},
      .external_order_id = std::string_view(),
  };

  auto found = _dispatcher.find_order(
      order_cancel_reject.cl_ord_id,
      order_cancel_reject.orig_cl_ord_id,
      order_lookup,
      trace_info,
      [&](const auto &order, auto &result) {
        DLOG_IF(FATAL, order.request_type != RequestType::MODIFY_ORDER)
        ("UNEXPECTED");

        result.origin = Origin::EXCHANGE;
        result.request_status = RequestStatus::REJECTED;
        result.error = fix::map_error(order_cancel_reject.text);
        result.text = order_cancel_reject.text;
      });

  if (found == false) {
    LOG(WARNING)("*** EXTERNAL ORDER ***");
    LOG(WARNING)("order_cancel_reject={}", order_cancel_reject);
  }
}

void Gateway::operator()(
    const fix::PositionReport &position_report,
    const server::TraceInfo &trace_info) {
  VLOG(1)(R"(position_report={})", position_report);
  switch (position_report.pos_req_result) {
    case core::fix::PosReqResult::VALID:
      switch (position_report.pos_req_type) {
        case core::fix::PosReqType::POSITIONS: {
          for (auto &position : position_report.no_positions) {
            PositionUpdate buy{
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
            PositionUpdate sell{
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
            server::create_trace_and_dispatch(
                trace_info, buy, _dispatcher, false);
            server::create_trace_and_dispatch(
                trace_info, sell, _dispatcher, true);
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
  // note! relaxed because we receive duplicate updates
  _fix.download.check_relaxed(FIXDownload::State::POSITIONS);
}

void Gateway::operator()(const fix::Reject &reject, const server::TraceInfo &) {
  LOG(WARNING)(R"(reject={})", reject);
  if (reject.session_reject_reason.compare("99") == 0 &&
      reject.text.compare("connection_too_slow") == 0) {
    _fix.connection.close();
  } else {
    LOG(FATAL)("Unexpected");
  }
}

void Gateway::operator()(
    const fix::SecurityList &security_list,
    const server::TraceInfo &trace_info) {
  _currencies.clear();
  if (security_list.no_related_sym.size() > 0) {
    assert(_symbols.empty());
    size_t security_count = 0;
    _symbols.reserve(security_list.no_related_sym.size());  // note! alloc
    for (auto &instrument : security_list.no_related_sym) {
      VLOG(1)(R"(instrument={})", instrument);
      // note!
      //   USD will cause a Reject
      //   using commission currency because it requires funding
      if (instrument.comm_currency.empty() == false)
        _currencies.emplace(instrument.comm_currency);
      if (_dispatcher.discard_symbol(instrument.symbol)) continue;
      _symbols.emplace_back(instrument.symbol);
      ReferenceData reference_data{
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
      server::create_trace_and_dispatch(
          trace_info, reference_data, _dispatcher, true);
      ++security_count;
    }
    VLOG(1)
    (R"(- securities: {} (/{}))",
     security_count,
     security_list.no_related_sym.size());
  }
  _fix.download.check(FIXDownload::State::SECURITIES);
}

void Gateway::operator()(
    const fix::SecurityStatus &, const server::TraceInfo &) {
}

void Gateway::operator()(
    const fix::UserResponse &user_response,
    const server::TraceInfo &trace_info) {
  FundsUpdate funds_update{
      .account = _account,
      .currency = user_response.currency,
      .balance = user_response.deribit_user_balance,
      .hold = double{0.0},
  };
  server::create_trace_and_dispatch(
      trace_info, funds_update, _dispatcher, true);
  _fix.download.check(FIXDownload::State::USER);
}

void Gateway::update(GatewayStatus gateway_status) {
  if (gateway_status == _gateway_status) return;
  _gateway_status = gateway_status;
  server::TraceInfo trace_info;
  MarketDataStatus market_data_status{
      .status = _gateway_status,
  };
  server::create_trace_and_dispatch(
      trace_info, market_data_status, _dispatcher, false);
  OrderManagerStatus order_manager_status{
      .account = _account,
      .status = _gateway_status,
  };
  server::create_trace_and_dispatch(
      trace_info, order_manager_status, _dispatcher, true);
  LOG(INFO)(R"(Update: gateway_status={})", _gateway_status);
}

void Gateway::download_securities() {
  auto request_id = _dispatcher.next_request_id();
  fix::SecurityListRequest security_list_request{
      .security_req_id = request_id,
      .security_list_request_type =
          core::fix::SecurityListRequestType::ALL_SECURITIES,
  };
  _fix.connection(security_list_request);
}

void Gateway::download_positions() {
  auto request_id = _dispatcher.next_request_id();
  fix::RequestForPositions request_for_positions{
      .pos_req_id = request_id,
      .pos_req_type = core::fix::PosReqType::POSITIONS,
      .subscription_request_type =
          roq::core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
      .currency = std::string_view(),
  };
  _fix.connection(request_for_positions);
}

void Gateway::download_orders() {
  auto request_id = _dispatcher.next_request_id();
  fix::OrderMassStatusRequest order_mass_status_request{
      .mass_status_req_id = request_id,
      .mass_status_req_type = core::fix::MassStatusReqType::ORDERS,
  };
  _fix.connection(order_mass_status_request);
}

void Gateway::download_user() {
  assert(_currencies.empty() == false);
  for (auto &currency : _currencies) {
    auto request_id = _dispatcher.next_request_id();
    fix::UserRequest user_request_btc{
        .user_request_id = request_id,
        .user_request_type =
            core::fix::UserRequestType::REQUEST_INDIVIDUAL_USER_STATUS,
        .username = _access_key,
        .currency = static_cast<std::string_view>(currency),
    };
    _fix.connection(user_request_btc);
  }
}

void Gateway::subscribe_market_data() {
  assert(_gateway_status == GatewayStatus::READY);
  if (_symbols.empty()) {
    LOG(WARNING)("Can't subscribe market data, reason: NO SYMBOLS");
    return;
  }
  LOG(INFO)("Subscribe market data");
  fix::MDReq md_entry_types[] = {
      {
          .md_entry_type = core::fix::MDEntryType::BID,
      },
      {
          .md_entry_type = core::fix::MDEntryType::OFFER,
      },
      {
          .md_entry_type = core::fix::MDEntryType::TRADE,
      }};
  std::vector<fix::InstrmtMDReq> related_sym(FLAGS_max_batch_size);
  for (size_t i = 0;; ++i) {
    auto offset = i * FLAGS_max_batch_size;
    if (_symbols.size() < offset) break;
    auto count =
        std::min<size_t>(_symbols.size() - offset, FLAGS_max_batch_size);
    if (count) {
      for (size_t j = 0; i < count; ++i)
        related_sym[j].symbol = _symbols[offset + j];
      auto request_id = _dispatcher.next_request_id();
      fix::MarketDataRequest market_data_request{
          .md_req_id = request_id,
          .subscription_request_type =
              core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES,
          .market_depth = 20,  // the maximum
          .md_update_type = core::fix::MDUpdateType::INCREMENTAL_REFRESH,
          .deribit_trade_amount = 0,      // none
          .deribit_since_timestamp = {},  // none
          .no_md_entry_types =
              roq::span(md_entry_types, std::size(md_entry_types)),
          .no_related_sym = roq::span(related_sym.data(), count),
      };
      _fix.connection(market_data_request);
    }
  }
}

}  // namespace deribit
}  // namespace roq
