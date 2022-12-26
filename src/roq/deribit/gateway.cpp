/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/gateway.hpp"

#include <utility>

#include "roq/deribit/flags/common.hpp"
#include "roq/deribit/flags/fix.hpp"
#include "roq/deribit/flags/multicast.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === HELPERS ===

namespace {
template <typename R>
auto create_security(auto const &config) {
  R result;
  for (auto &[_, account] : config.accounts)
    result.try_emplace(account.name, std::make_unique<Security>(config, account.name));
  return result;
}

auto &get_security(auto const &security_by_name, auto const &master_account) {
  auto iter = security_by_name.find(master_account);
  if (iter != security_by_name.end())
    return *(*iter).second;
  log::fatal("Market data requires a master account"sv);
}

template <typename R>
auto create_order_entry(auto &gateway, auto &context, auto &stream_id, auto &security_by_name, auto &shared) {
  R result;
  for (auto &[account, security] : security_by_name)
    result.try_emplace(account, std::make_unique<OrderEntry>(gateway, context, ++stream_id, *security, shared));
  return result;
}

template <typename R>
auto create_drop_copy(auto &gateway, auto &context, auto &stream_id, auto &security_by_name, auto &shared) {
  R result;
  for (auto &[account, security] : security_by_name)
    result.try_emplace(account, std::make_unique<DropCopy>(gateway, context, ++stream_id, *security, shared));
  return result;
}

template <typename R>
auto create_web_socket(auto &gateway, auto &context, auto &stream_id, auto &shared) {
  R result;
  result.emplace_back(std::make_unique<WebSocket>(gateway, context, ++stream_id, shared, std::size(result), true));
  return result;
}

template <typename R>
auto create_market_data(auto &gateway, auto &context, auto &stream_id, auto &security, auto &shared) {
  R result;
  result.emplace_back(
      std::make_unique<MarketData>(gateway, context, stream_id, security, shared, std::size(result), true));
  return result;
}

auto create_udp_snapshot(auto &gateway, auto &context, auto &stream_id, auto &shared) {
  if (shared.has_multicast())
    return std::make_unique<UDPSnapshot>(gateway, context, stream_id, shared);
  return std::unique_ptr<UDPSnapshot>{};
}

auto create_udp_events(auto &gateway, auto &context, auto &stream_id, auto &shared) {
  if (shared.has_multicast())
    return std::make_unique<UDPEvents>(gateway, context, stream_id, shared);
  return std::unique_ptr<UDPEvents>{};
}
}  // namespace

// === IMPLEMENTATION ===

Gateway::Gateway(server::Dispatcher &dispatcher, Config const &config, io::Context &context)
    : dispatcher_{dispatcher}, master_account_{config.get_master_account()},
      security_{create_security<decltype(security_)>(config)}, context_{context}, shared_{dispatcher_},
      order_entry_{create_order_entry<decltype(order_entry_)>(*this, context_, stream_id_, security_, shared_)},
      drop_copy_{create_drop_copy<decltype(drop_copy_)>(*this, context_, stream_id_, security_, shared_)},
      web_socket_{create_web_socket<decltype(web_socket_)>(*this, context_, stream_id_, shared_)},
      market_data_{create_market_data<decltype(market_data_)>(
          *this, context_, ++stream_id_, get_security(security_, master_account_), shared_)},
      udp_snapshot_{create_udp_snapshot(*this, context_, ++stream_id_, shared_)},
      udp_events_{create_udp_events(*this, context_, ++stream_id_, shared_)} {
  if (std::empty(master_account_) && !flags::Common::disable_master_account_check())
    log::fatal("A master account is always required (due to FIX logon)"sv);
  if (!flags::FIX::fix_cancel_on_disconnect())
    log::warn("Orders will *NOT* be cancelled on disconnect"sv);
}

void Gateway::operator()(Event<Start> const &event) {
  log::info("Starting..."sv);
  for (auto &[_, iter] : order_entry_)
    (*iter)(event);
  for (auto &[_, iter] : drop_copy_)
    (*iter)(event);
  for (auto &iter : web_socket_)
    (*iter)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
  if (udp_snapshot_)
    (*udp_snapshot_)(event);
  if (udp_events_)
    (*udp_events_)(event);
}

void Gateway::operator()(Event<Stop> const &event) {
  log::info("Stopping..."sv);
  if (udp_events_)
    (*udp_events_)(event);
  if (udp_snapshot_)
    (*udp_snapshot_)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
  for (auto &iter : web_socket_)
    (*iter)(event);
  for (auto &[_, iter] : drop_copy_)
    (*iter)(event);
  for (auto &[_, iter] : order_entry_)
    (*iter)(event);
}

void Gateway::operator()(Event<Timer> const &event) {
  for (auto &[_, iter] : order_entry_)
    (*iter)(event);
  for (auto &[_, iter] : drop_copy_)
    (*iter)(event);
  for (auto &iter : web_socket_)
    (*iter)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
  if (udp_snapshot_)
    (*udp_snapshot_)(event);
  if (udp_events_)
    (*udp_events_)(event);
}

void Gateway::operator()(Event<Connected> const &) {
}

void Gateway::operator()(Event<Disconnected> const &event) {
  auto const &[message_info, disconnected] = event;
  log::warn(
      R"(Disconnected: source="{}", order_cancel_policy={})"sv,
      message_info.source_name,
      disconnected.order_cancel_policy);
  switch (disconnected.order_cancel_policy) {
    using enum OrderCancelPolicy;
    case UNDEFINED:
      break;
    case MANAGED_ORDERS:
      log::warn("*** CANCEL MANAGED ORDERS NOT IMPLEMENTED ***"sv);
      break;
    case BY_ACCOUNT:
      log::warn("*** CANCEL ALL ACCOUNT ORDERS ***"sv);
      for (auto &[account, order_entry] : order_entry_) {
        if (dispatcher_.can_user_trade_account(account, message_info.source)) {
          log::warn(R"(- account="{}")"sv, account);
          CancelAllOrders cancel_all_orders{
              .account = account,
          };
          Event event{message_info, cancel_all_orders};
          (*order_entry)(event, {});
        }
      }
  }
}

uint16_t Gateway::operator()(
    Event<CreateOrder> const &event, oms::Order const &order, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, order, request_id);
}

uint16_t Gateway::operator()(
    Event<ModifyOrder> const &event,
    oms::Order const &order,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, request_id, previous_request_id);
}

uint16_t Gateway::operator()(
    Event<CancelOrder> const &event,
    oms::Order const &order,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, request_id, previous_request_id);
}

uint16_t Gateway::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, request_id);
}

void Gateway::operator()(metrics::Writer &writer) {
  for (auto &[_, iter] : order_entry_)
    (*iter)(writer);
  for (auto &[_, iter] : drop_copy_)
    (*iter)(writer);
  for (auto &iter : web_socket_)
    (*iter)(writer);
  for (auto &iter : market_data_)
    (*iter)(writer);
  if (udp_snapshot_)
    (*udp_snapshot_)(writer);
  if (udp_events_)
    (*udp_events_)(writer);
}

void Gateway::operator()(Trace<StreamStatus> const &event) {
  dispatcher_(event);
}

void Gateway::operator()(Trace<ExternalLatency> const &event) {
  dispatcher_(event);
}

void Gateway::operator()(Trace<ReferenceData> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<MarketStatus> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<TopOfBook> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<MarketByPriceUpdate> const &event, bool is_last, bool refresh) {
  dispatcher_(
      event, is_last, refresh, shared_.final_bids, shared_.final_asks, []([[maybe_unused]] auto &market_by_price) {});
}

void Gateway::operator()(Trace<TradeSummary> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<StatisticsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<TradeUpdate> const &event, bool is_last, uint8_t user_id) {
  dispatcher_(event, is_last, user_id);
}

void Gateway::operator()(Trace<PositionUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<FundsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(WebSocket::SymbolsUpdate &symbols_update) {
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(size);
  for (auto &iter : market_data_)
    (*iter).subscribe(start_from);
  for (auto &iter : web_socket_)
    (*iter).subscribe(start_from);
}

void Gateway::operator()(WebSocket::CurrenciesUpdate &currencies_update) {
  auto &currencies = currencies_update.currencies;
  for (auto &[_, iter] : drop_copy_) {
    (*iter).update_subscriptions(currencies);
  }
}

void Gateway::operator()(MarketData::SymbolsUpdate &symbols_update) {
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(size);
  for (auto &iter : market_data_)
    (*iter).subscribe(start_from);
  for (auto &iter : web_socket_)
    (*iter).subscribe(start_from);
}

void Gateway::ensure_symbol_slices(size_t size) {
  // market data
  while (std::size(market_data_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(market_data_);
    log::debug("Create MarketData(stream_id={}, index={})"sv, stream_id, index);
    auto market_data =
        std::make_unique<MarketData>(*this, context_, stream_id, *security_[master_account_], shared_, index, false);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*market_data, message_info, start);
    market_data_.emplace_back(std::move(market_data));
  }
  // web socket
  while (std::size(web_socket_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(web_socket_);
    log::debug("Create WebSocket (stream_id={}, index={})"sv, stream_id, index);
    auto web_socket = std::make_unique<WebSocket>(*this, context_, stream_id, shared_, index, false);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*web_socket, message_info, start);
    web_socket_.emplace_back(std::move(web_socket));
  }
}

OrderEntry &Gateway::get_order_entry(std::string_view const &account) {
  auto iter = order_entry_.find(account);
  if (iter == std::end(order_entry_)) [[unlikely]]
    throw RuntimeError{R"(Unknown account="{}")"sv, account};
  return *(*iter).second;
}

}  // namespace deribit
}  // namespace roq
