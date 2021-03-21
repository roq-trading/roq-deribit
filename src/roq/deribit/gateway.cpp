/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <utility>

#include "roq/deribit/flags.h"

using namespace roq::literals;

namespace roq {
namespace deribit {

namespace {
static auto create_security(const Config &config) {
  absl::flat_hash_map<std::string, std::unique_ptr<Security>> result;
  for (auto &[_, iter] : config.accounts) {
    auto &account = iter.name;
    auto &access_key = iter.login;
    auto &access_secret = iter.secret;
    result.try_emplace(account, std::make_unique<Security>(account, access_key, access_secret));
  }
  return result;
}

template <typename T>
static auto create_order_entry(
    Gateway &gateway,
    core::io::Context &context,
    uint16_t &stream_id,
    T &security,
    Shared &shared) {
  absl::flat_hash_map<std::string, std::unique_ptr<OrderEntry>> result;
  for (auto &iter : security) {
    result.try_emplace(
        iter.first,
        std::make_unique<OrderEntry>(gateway, context, ++stream_id, *iter.second, shared));
  }
  return result;
}

template <typename T>
static auto create_drop_copy(
    Gateway &gateway,
    core::io::Context &context,
    uint16_t &stream_id,
    T &security,
    Shared &shared) {
  absl::flat_hash_map<std::string, std::unique_ptr<DropCopy>> result;
  for (auto &iter : security) {
    result.try_emplace(
        iter.first,
        std::make_unique<DropCopy>(gateway, context, ++stream_id, *iter.second, shared));
  }
  return result;
}

static auto create_web_socket(
    Gateway &gateway, core::io::Context &context, uint16_t &stream_id, Shared &shared) {
  std::list<std::unique_ptr<WebSocket>> result;
  result.emplace_back(std::make_unique<WebSocket>(gateway, context, ++stream_id, shared, true));
  return result;
}

static auto create_market_data(
    Gateway &gateway,
    core::io::Context &context,
    uint16_t &stream_id,
    Security &security,
    Shared &shared) {
  std::list<std::unique_ptr<MarketData>> result;
  result.emplace_back(
      std::make_unique<MarketData>(gateway, context, stream_id, security, shared, true));
  return result;
}
}  // namespace

Gateway::Gateway(server::Dispatcher &dispatcher, const Config &config)
    : dispatcher_(dispatcher), master_account_(config.get_master_account()),
      security_(create_security(config)), shared_(dispatcher_),
      order_entry_(create_order_entry(*this, context_, stream_id_, security_, shared_)),
      drop_copy_(create_drop_copy(*this, context_, stream_id_, security_, shared_)),
      web_socket_(create_web_socket(*this, context_, stream_id_, shared_)),
      market_data_(
          create_market_data(*this, context_, ++stream_id_, *security_[master_account_], shared_)) {
  if (!Flags::fix_cancel_on_disconnect())
    log::warn("Orders will *NOT* be cancelled on disconnect"_sv);
}

void Gateway::operator()(const Event<Start> &event) {
  log::info("Starting the gateway..."_sv);
  for (auto &[_, iter] : order_entry_)
    (*iter)(event);
  for (auto &[_, iter] : drop_copy_)
    (*iter)(event);
  for (auto &iter : web_socket_)
    (*iter)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
}

void Gateway::operator()(const Event<Stop> &event) {
  log::info("Stopping the gateway..."_sv);
  for (auto &iter : market_data_)
    (*iter)(event);
  for (auto &iter : web_socket_)
    (*iter)(event);
  for (auto &[_, iter] : drop_copy_)
    (*iter)(event);
  for (auto &[_, iter] : order_entry_)
    (*iter)(event);
}

void Gateway::operator()(const Event<Timer> &event) {
  for (auto &[_, iter] : order_entry_)
    (*iter)(event);
  for (auto &[_, iter] : drop_copy_)
    (*iter)(event);
  for (auto &iter : web_socket_)
    (*iter)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
  context_.dispatch(true);
}

void Gateway::operator()(const Event<Connection> &) {
}

void Gateway::operator()(
    const Event<CreateOrder> &event,
    const std::string_view &request_id,
    uint32_t gateway_order_id) {
  assert(!event.value.account.empty());
  get_order_entry(event.value.account)(event, request_id, gateway_order_id);
}

void Gateway::operator()(
    const Event<ModifyOrder> &event,
    const std::string_view &request_id,
    const server::OMS_Order &order) {
  assert(!event.value.account.empty());
  assert(event.value.account == order.account);
  get_order_entry(event.value.account)(event, request_id, order);
}

void Gateway::operator()(
    const Event<CancelOrder> &event,
    const std::string_view &request_id,
    const server::OMS_Order &order) {
  assert(!event.value.account.empty());
  assert(event.value.account == order.account);
  get_order_entry(event.value.account)(event, request_id, order);
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
}

void Gateway::operator()(const server::Trace<StreamUpdate> &event) {
  dispatcher_(event);
}

void Gateway::operator()(const server::Trace<ExternalLatency> &event) {
  dispatcher_(event);
}

void Gateway::operator()(const server::Trace<ReferenceData> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<MarketStatus> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<TopOfBook> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<MarketByPriceUpdate> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<TradeSummary> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<StatisticsUpdate> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<TradeUpdate> &event, bool is_last, uint8_t user_id) {
  dispatcher_(event, is_last, user_id);
}

void Gateway::operator()(const server::Trace<PositionUpdate> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<FundsUpdate> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(WebSocket::SymbolsUpdate &symbols_update) {
  auto &symbols = symbols_update.symbols;
  for (auto &iter : web_socket_) {
    if (symbols.empty())
      break;
    (*iter).update_subscriptions(symbols);
  }
  for (;;) {
    if (symbols.empty())
      break;
    auto web_socket = std::make_unique<WebSocket>(*this, context_, ++stream_id_, shared_, false);
    (*web_socket).update_subscriptions(symbols);
    MessageInfo message_info;  // XXX something sensible
    Start start;
    create_event_and_dispatch(*web_socket, message_info, start);
    web_socket_.emplace_back(std::move(web_socket));
  }
}

void Gateway::operator()(WebSocket::CurrenciesUpdate &currencies_update) {
  auto &currencies = currencies_update.currencies;
  for (auto &[_, iter] : drop_copy_) {
    (*iter).update_subscriptions(currencies);
  }
}

void Gateway::operator()(MarketData::SymbolsUpdate &symbols_update) {
  auto &symbols = symbols_update.symbols;
  for (auto &iter : market_data_) {
    if (symbols.empty())
      break;
    (*iter).update_subscriptions(symbols);
  }
  for (;;) {
    if (symbols.empty())
      break;
    auto market_data = std::make_unique<MarketData>(
        *this, context_, ++stream_id_, *security_[master_account_], shared_, false);
    (*market_data).update_subscriptions(symbols);
    MessageInfo message_info;  // XXX something sensible
    Start start;
    create_event_and_dispatch(*market_data, message_info, start);
    market_data_.emplace_back(std::move(market_data));
  }
}

OrderEntry &Gateway::get_order_entry(const std::string_view &account) {
  auto iter = order_entry_.find(account);
  if (iter != order_entry_.end())
    return *(*iter).second;
  throw std::runtime_error(roq::format(R"(Unknown account="{}")"_fmt, account));
}

}  // namespace deribit
}  // namespace roq
