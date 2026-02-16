/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/gateway.hpp"

#include <utility>

#include "roq/server/oms/exceptions.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

// === HELPERS ===

namespace {
auto create_master_account(auto &config) {
  std::string_view result = config.get_master_account();
  if (std::empty(result)) {
    log::fatal("Market data requires a master account"sv);
  }
  return result;
}

template <typename R>
R create_accounts(auto &config) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, account] : config.accounts) {
    auto obj = std::make_unique<Account>(config, account.name);
    result.try_emplace(static_cast<std::string_view>(account.name), std::move(obj));
  }
  return result;
}

template <typename R>
R create_order_entry(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, item] : accounts) {
    auto &account = *item;
    auto obj = std::make_unique<OrderEntry>(gateway, context, ++stream_id, account, shared);
    result.try_emplace(static_cast<std::string_view>(account.name), std::move(obj));
  }
  return result;
}

template <typename R>
R create_drop_copy(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, item] : accounts) {
    auto &account = *item;
    auto obj = std::make_unique<DropCopy>(gateway, context, ++stream_id, account, shared);
    result.try_emplace(static_cast<std::string_view>(account.name), std::move(obj));
  }
  return result;
}

template <typename R>
R create_web_socket(auto &gateway, auto &context, auto &stream_id, auto &account, auto &shared, auto &request) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto obj = std::make_unique<WebSocket>(gateway, context, ++stream_id, account, shared, request, std::size(result), true);
  result.emplace_back(std::move(obj));
  return result;
}

template <typename R>
R create_market_data(auto &gateway, auto &context, auto &stream_id, auto &account, auto &shared) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto obj = std::make_unique<MarketData>(gateway, context, stream_id, account, shared, std::size(result), true);
  result.emplace_back(std::move(obj));
  return result;
}

auto create_udp_snapshot(auto &gateway, auto &context, auto &stream_id, auto &shared) {
  if (shared.has_multicast()) {
    return std::make_unique<UDPSnapshot>(gateway, context, stream_id, shared);
  }
  return std::unique_ptr<UDPSnapshot>{};
}

auto create_udp_events(auto &gateway, auto &context, auto &stream_id, auto &shared) {
  if (shared.has_multicast()) {
    return std::make_unique<UDPEvents>(gateway, context, stream_id, shared);
  }
  return std::unique_ptr<UDPEvents>{};
}
}  // namespace

// === IMPLEMENTATION ===

Gateway::Gateway(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context)
    : dispatcher_{dispatcher}, master_account_{create_master_account(config)}, accounts_{create_accounts<decltype(accounts_)>(config)}, context_{context},
      shared_{dispatcher_, settings}, rest_{*this, context_, ++stream_id_, shared_, request_},
      order_entry_{create_order_entry<decltype(order_entry_)>(*this, context_, stream_id_, accounts_, shared_)},
      drop_copy_{create_drop_copy<decltype(drop_copy_)>(*this, context_, stream_id_, accounts_, shared_)},
      web_socket_{create_web_socket<decltype(web_socket_)>(*this, context_, stream_id_, get_account(master_account_), shared_, request_)},
      market_data_{create_market_data<decltype(market_data_)>(*this, context_, ++stream_id_, get_account(master_account_), shared_)},
      udp_snapshot_{create_udp_snapshot(*this, context_, ++stream_id_, shared_)}, udp_events_{create_udp_events(*this, context_, ++stream_id_, shared_)} {
  if (std::empty(master_account_) && !settings.misc.disable_master_account_check) {
    log::fatal("A master account is always required (due to FIX logon)"sv);
  }
  if (!settings.fix.cancel_on_disconnect) {
    log::warn("Orders will *NOT* be cancelled on disconnect"sv);
  }
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

void Gateway::operator()(Event<Control> const &event) {
  auto &[message_info, control] = event;
  switch (control.action) {
    using enum Action;
    case UNDEFINED:
      assert(false);
      break;
    case ENABLE:
      dispatcher_(State::ENABLED);
      break;
    case DISABLE:
      dispatcher_(State::DISABLED);
      break;
  }
}

void Gateway::operator()(Event<Connected> const &) {
}

void Gateway::operator()(Event<Disconnected> const &) {
}

void Gateway::operator()(Event<Subscribe> const &event) {
  auto &[message_info, subscribe] = event;
  std::vector<Symbol> symbols;
  for (auto &item : subscribe.symbols) {
    if (shared_.all_symbols.emplace(item).second) {
      symbols.emplace_back(item);
    } else {
      log::warn(R"(*** DUPLICATE SUBSCRIPTION *** (symbol="{}")"sv, item);
    }
  }
  auto symbols_update = Rest::SymbolsUpdate{
      .symbols = symbols,
  };
  (*this)(symbols_update);
}

uint16_t Gateway::operator()(
    Event<CreateOrder> const &event, server::oms::Order const &order, server::oms::RefData const &ref_data, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, order, ref_data, request_id);
}

uint16_t Gateway::operator()(
    Event<ModifyOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, ref_data, request_id, previous_request_id);
}

uint16_t Gateway::operator()(
    Event<CancelOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, ref_data, request_id, previous_request_id);
}

uint16_t Gateway::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, request_id);
}

uint16_t Gateway::operator()(Event<MassQuote> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Gateway::operator()(Event<CancelQuotes> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

void Gateway::operator()(metrics::Writer &writer) const {
  dispatch_helper(*this, writer);
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

void Gateway::operator()(Trace<TimeSeriesUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<TradeUpdate> const &event, bool is_last, uint8_t user_id, std::string_view const &request_id) {
  dispatcher_(event, is_last, user_id, request_id);
}

void Gateway::operator()(Trace<PositionUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<FundsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Rest::CurrenciesUpdate &currencies_update) {
  auto &currencies = currencies_update.currencies;
  for (auto &[_, iter] : drop_copy_) {
    (*iter).update_subscriptions(currencies);
  }
}

void Gateway::operator()(Rest::SymbolsUpdate &symbols_update) {
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(size);
  for (auto &item : market_data_) {
    (*item).subscribe(start_from);
  }
  for (auto &item : web_socket_) {
    (*item).subscribe(start_from);
  }
}

void Gateway::operator()(WebSocket::Latch const &) {
  for (auto &[_, item] : drop_copy_) {
    (*item).download();
  }
}

void Gateway::operator()(MarketData::SymbolsUpdate &symbols_update) {
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(size);
  for (auto &item : market_data_) {
    (*item).subscribe(start_from);
  }
  for (auto &item : web_socket_) {
    (*item).subscribe(start_from);
  }
}

void Gateway::ensure_symbol_slices(size_t size) {
  // market data
  while (std::size(market_data_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(market_data_);
    log::info("Create MarketData(stream_id={}, index={})"sv, stream_id, index);
    auto market_data = std::make_unique<MarketData>(*this, context_, stream_id, get_account(master_account_), shared_, index, false);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*market_data, message_info, start);
    market_data_.emplace_back(std::move(market_data));
  }
  // web socket
  while (std::size(web_socket_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(web_socket_);
    log::info("Create WebSocket (stream_id={}, index={})"sv, stream_id, index);
    auto web_socket = std::make_unique<WebSocket>(*this, context_, stream_id, get_account(master_account_), shared_, request_, index, false);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*web_socket, message_info, start);
    web_socket_.emplace_back(std::move(web_socket));
  }
}

template <typename... Args>
void Gateway::dispatch(Args &&...args) {
  dispatch_helper(*this, std::forward<Args>(args)...);
}

template <typename... Args>
void Gateway::dispatch_helper(auto &self, Args &&...args) {
  auto helper = [&](auto &target) { target(std::forward<Args>(args)...); };
  helper(self.rest_);
  for (auto &[_, item] : self.order_entry_) {
    helper(*item);
  }
  for (auto &[_, item] : self.drop_copy_) {
    helper(*item);
  }
  for (auto &item : self.web_socket_) {
    helper(*item);
  }
  for (auto &item : self.market_data_) {
    helper(*item);
  }
  if (self.udp_snapshot_) {
    helper(*self.udp_snapshot_);
  }
  if (self.udp_events_) {
    helper(*self.udp_events_);
  }
}

Account &Gateway::get_account(std::string_view const &account) {
  auto iter = accounts_.find(account);
  if (iter == std::end(accounts_)) [[unlikely]] {
    throw RuntimeError{R"(Unknown account="{}")"sv, account};
  }
  return *(*iter).second;
}

OrderEntry &Gateway::get_order_entry(std::string_view const &account) {
  auto iter = order_entry_.find(account);
  if (iter == std::end(order_entry_)) [[unlikely]] {
    throw RuntimeError{R"(Unknown account="{}")"sv, account};
  }
  return *(*iter).second;
}

}  // namespace deribit
}  // namespace roq
