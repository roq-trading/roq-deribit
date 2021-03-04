/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <utility>

#include "roq/deribit/flags.h"

using namespace roq::literals;

namespace roq {
namespace deribit {

namespace {
static auto create_security(const Config &config) {
  auto account = config.get_account();
  absl::flat_hash_map<std::string, std::unique_ptr<Security>> result;
  result.try_emplace(account, std::make_unique<Security>(config, account));
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

static auto create_market_data(
    Gateway &gateway,
    core::io::Context &context,
    uint16_t &stream_id,
    Security &security,
    Shared &shared) {
  std::list<std::unique_ptr<MarketData>> result;
  result.emplace_back(std::make_unique<MarketData>(gateway, context, stream_id, security, shared));
  return result;
}
}  // namespace

Gateway::Gateway(server::Dispatcher &dispatcher, const Config &config)
    : dispatcher_(dispatcher), master_account_(config.get_account()),
      security_(create_security(config)), shared_(dispatcher_),
      web_socket_(*this, context_, ++stream_id_, *security_[master_account_], shared_),
      order_entry_(create_order_entry(*this, context_, stream_id_, security_, shared_)),
      market_data_(
          create_market_data(*this, context_, ++stream_id_, *security_[master_account_], shared_)) {
  LOG_IF(WARNING, Flags::fix_cancel_on_disconnect() == false)
  ("Orders will *NOT* be cancelled on disconnect"_sv);
}

void Gateway::operator()(const Event<Start> &event) {
  LOG(INFO)("Starting the gateway..."_sv);
  web_socket_(event);
  for (auto &[_, iter] : order_entry_)
    (*iter)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
}

void Gateway::operator()(const Event<Stop> &event) {
  LOG(INFO)("Stopping the gateway..."_sv);
  web_socket_(event);
  for (auto &[_, iter] : order_entry_)
    (*iter)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
}

void Gateway::operator()(const Event<Timer> &event) {
  for (auto &iter : market_data_)
    (*iter)(event);
  for (auto &[_, iter] : order_entry_)
    (*iter)(event);
  web_socket_(event);
  context_.dispatch(true);
}

void Gateway::operator()(const Event<Connection> &) {
}

void Gateway::operator()(
    const Event<CreateOrder> &event,
    const std::string_view &request_id,
    uint32_t gateway_order_id) {
  assert(event.value.account.empty() == false);
  get_order_entry(event.value.account)(event, request_id, gateway_order_id);
}

void Gateway::operator()(
    const Event<ModifyOrder> &event,
    const std::string_view &request_id,
    const server::OMS_Order &order) {
  assert(event.value.account.empty() == false);
  assert(event.value.account == order.account);
  get_order_entry(event.value.account)(event, request_id, order);
}

void Gateway::operator()(
    const Event<CancelOrder> &event,
    const std::string_view &request_id,
    const server::OMS_Order &order) {
  assert(event.value.account.empty() == false);
  assert(event.value.account == order.account);
  get_order_entry(event.value.account)(event, request_id, order);
}

void Gateway::operator()(metrics::Writer &writer) {
  web_socket_(writer);
  for (auto &[_, iter] : order_entry_)
    (*iter)(writer);
  for (auto &iter : market_data_)
    (*iter)(writer);
}

void Gateway::operator()(const server::Trace<ExternalLatency> &event) {
  dispatcher_(event);
}

void Gateway::operator()(const server::Trace<MarketDataStatus> &event) {
  dispatcher_(event, true);
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

void Gateway::operator()(const server::Trace<OrderManagerStatus> &event) {
  dispatcher_(event, true);
}

void Gateway::operator()(const server::Trace<OrderAck> &event, bool is_last, uint8_t user_id) {
  dispatcher_(event, is_last, user_id);
}

void Gateway::operator()(const server::Trace<OrderUpdate> &event, bool is_last, uint8_t user_id) {
  dispatcher_(event, is_last, user_id);
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

void Gateway::operator()(MarketData::Refresh &refresh) {
  auto &symbols = refresh.symbols;
  for (auto &iter : market_data_) {
    if (symbols.empty())
      break;
    (*iter)(refresh);
  }
  for (;;) {
    if (symbols.empty())
      break;
    assert(!market_data_.empty());
    auto market_data = std::make_unique<MarketData>(
        *this, context_, ++stream_id_, *security_[master_account_], shared_, refresh);
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
