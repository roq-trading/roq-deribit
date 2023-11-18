/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/gateway.hpp"

#include <utility>

using namespace std::literals;

namespace roq {
namespace deribit {

// === HELPERS ===

namespace {
template <typename R>
R create_accounts(auto &config) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  for (auto &[_, account] : config.accounts)
    result.try_emplace(account.name, std::make_unique<Account>(config, account.name));
  return result;
}

auto &get_account(auto &accounts, auto &master_account) {
  auto iter = accounts.find(master_account);
  if (iter != std::end(accounts))
    return *(*iter).second;
  log::fatal("Market data requires a master account"sv);
}

template <typename R>
R create_order_entry(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  for (auto &[name, account] : accounts)
    result.try_emplace(name, std::make_unique<OrderEntry>(gateway, context, ++stream_id, *account, shared));
  return result;
}

template <typename R>
R create_drop_copy(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  for (auto &[name, account] : accounts)
    result.try_emplace(name, std::make_unique<DropCopy>(gateway, context, ++stream_id, *account, shared));
  return result;
}

template <typename R>
R create_web_socket(auto &gateway, auto &context, auto &stream_id, auto &account, auto &shared) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  result.emplace_back(
      std::make_unique<WebSocket>(gateway, context, ++stream_id, account, shared, std::size(result), true));
  return result;
}

template <typename R>
R create_market_data(auto &gateway, auto &context, auto &stream_id, auto &account, auto &shared) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  result.emplace_back(
      std::make_unique<MarketData>(gateway, context, stream_id, account, shared, std::size(result), true));
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

Gateway::Gateway(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context)
    : dispatcher_{dispatcher}, master_account_{config.get_master_account()},
      accounts_{create_accounts<decltype(accounts_)>(config)}, context_{context}, shared_{dispatcher_, settings},
      order_entry_{create_order_entry<decltype(order_entry_)>(*this, context_, stream_id_, accounts_, shared_)},
      drop_copy_{create_drop_copy<decltype(drop_copy_)>(*this, context_, stream_id_, accounts_, shared_)},
      web_socket_{create_web_socket<decltype(web_socket_)>(
          *this, context_, stream_id_, get_account(accounts_, master_account_), shared_)},
      market_data_{create_market_data<decltype(market_data_)>(
          *this, context_, ++stream_id_, get_account(accounts_, master_account_), shared_)},
      udp_snapshot_{create_udp_snapshot(*this, context_, ++stream_id_, shared_)},
      udp_events_{create_udp_events(*this, context_, ++stream_id_, shared_)} {
  if (std::empty(master_account_) && !settings.common.disable_master_account_check)
    log::fatal("A master account is always required (due to FIX logon)"sv);
  if (!settings.fix.cancel_on_disconnect)
    log::warn("Orders will *NOT* be cancelled on disconnect"sv);
}

void Gateway::operator()(Event<Start> const &event) {
  log::info("Starting..."sv);
  dispatch(event);
}

void Gateway::operator()(Event<Stop> const &event) {
  log::info("Stopping..."sv);
  dispatch(event);
}

void Gateway::operator()(Event<Timer> const &event) {
  dispatch(event);
}

void Gateway::operator()(Event<server::Refresh> const &) {
}

void Gateway::operator()(Event<Connected> const &) {
}

void Gateway::operator()(Event<Disconnected> const &) {
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
  dispatch(writer);
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

void Gateway::operator()(Trace<MarketByPriceUpdate> const &event, bool is_last) {
  auto callback = []([[maybe_unused]] auto &market_by_price) {};
  dispatcher_(event, is_last, bids_, asks_, callback);
}

void Gateway::operator()(Trace<TradeSummary> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<StatisticsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(
    Trace<TradeUpdate> const &event, bool is_last, uint8_t user_id, std::string_view const &request_id) {
  dispatcher_(event, is_last, user_id, request_id);
}

void Gateway::operator()(Trace<PositionUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<FundsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(WebSocket::CurrenciesUpdate &currencies_update) {
  auto &currencies = currencies_update.currencies;
  for (auto &[_, iter] : drop_copy_)
    (*iter).update_subscriptions(currencies);
}

void Gateway::operator()(WebSocket::SymbolsUpdate &symbols_update) {
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(size);
  for (auto &iter : market_data_)
    (*iter).subscribe(start_from);
  for (auto &iter : web_socket_)
    (*iter).subscribe(start_from);
}

void Gateway::operator()(WebSocket::Latch const &) {
  for (auto &[_, iter] : drop_copy_)
    (*iter).download();
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
        std::make_unique<MarketData>(*this, context_, stream_id, *accounts_.at(master_account_), shared_, index, false);
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
    auto web_socket =
        std::make_unique<WebSocket>(*this, context_, stream_id, *accounts_.at(master_account_), shared_, index, false);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*web_socket, message_info, start);
    web_socket_.emplace_back(std::move(web_socket));
  }
}

template <typename... Args>
void Gateway::dispatch(Args &&...args) {
  auto helper = [&](auto &target) { target(std::forward<Args>(args)...); };
  for (auto &[_, item] : order_entry_)
    helper(*item);
  for (auto &[_, item] : drop_copy_)
    helper(*item);
  for (auto &item : web_socket_)
    helper(*item);
  for (auto &item : market_data_)
    helper(*item);
  if (udp_snapshot_)
    helper(*udp_snapshot_);
  if (udp_events_)
    helper(*udp_events_);
}

OrderEntry &Gateway::get_order_entry(std::string_view const &account) {
  auto iter = order_entry_.find(account);
  if (iter == std::end(order_entry_)) [[unlikely]]
    throw RuntimeError{R"(Unknown account="{}")"sv, account};
  return *(*iter).second;
}

}  // namespace deribit
}  // namespace roq
