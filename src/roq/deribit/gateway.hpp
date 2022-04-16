/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <vector>

#include "roq/server.hpp"

#include "roq/core/io/context.hpp"

#include "roq/deribit/config.hpp"
#include "roq/deribit/drop_copy.hpp"
#include "roq/deribit/market_data.hpp"
#include "roq/deribit/multicast.hpp"
#include "roq/deribit/order_entry.hpp"
#include "roq/deribit/security.hpp"
#include "roq/deribit/shared.hpp"
#include "roq/deribit/web_socket.hpp"

namespace roq {
namespace deribit {

class Gateway final : public server::Handler,
                      public OrderEntry::Handler,
                      public DropCopy::Handler,
                      public WebSocket::Handler,
                      public MarketData::Handler,
                      public Multicast::Handler {
 public:
  Gateway(server::Dispatcher &, const Config &);

  Gateway(Gateway &&) = delete;
  Gateway(const Gateway &) = delete;

 protected:
  // server::Handler

  void operator()(const Event<Start> &) override;
  void operator()(const Event<Stop> &) override;
  void operator()(const Event<Timer> &) override;
  void operator()(const Event<Connected> &) override;
  void operator()(const Event<Disconnected> &) override;

  uint16_t operator()(
      const Event<CreateOrder> &, const oms::Order &, const std::string_view &request_id) override;
  uint16_t operator()(
      const Event<ModifyOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id) override;
  uint16_t operator()(
      const Event<CancelOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id) override;

  uint16_t operator()(const Event<CancelAllOrders> &, const std::string_view &request_id) override;

  void operator()(metrics::Writer &) override;

  // many

  void operator()(const Trace<StreamStatus> &) override;
  void operator()(const Trace<ExternalLatency> &) override;
  void operator()(const Trace<ReferenceData> &, bool is_last) override;
  void operator()(const Trace<MarketStatus> &, bool is_last) override;
  void operator()(const Trace<TopOfBook> &, bool is_last) override;
  void operator()(const Trace<MarketByPriceUpdate> &, bool is_last, bool refresh) override;
  void operator()(const Trace<TradeSummary> &, bool is_last) override;
  void operator()(const Trace<StatisticsUpdate> &, bool is_last) override;
  void operator()(const Trace<TradeUpdate> &, bool is_last, uint8_t user_id) override;
  void operator()(const Trace<PositionUpdate> &, bool is_last) override;
  void operator()(const Trace<FundsUpdate> &, bool is_last) override;

  void operator()(WebSocket::CurrenciesUpdate &) override;
  void operator()(WebSocket::SymbolsUpdate &) override;

  void operator()(MarketData::SymbolsUpdate &) override;

  void ensure_symbol_slices(size_t size);

  // utilities

  OrderEntry &get_order_entry(const std::string_view &account);

 private:
  server::Dispatcher &dispatcher_;
  // config
  const std::string master_account_;
  // security
  absl::flat_hash_map<Account, std::unique_ptr<Security>> security_;
  // io
  core::io::Context context_;
  // shared
  Shared shared_;
  // seed
  uint16_t stream_id_ = {};
  // streams
  absl::flat_hash_map<Account, std::unique_ptr<OrderEntry>> order_entry_;
  absl::flat_hash_map<Account, std::unique_ptr<DropCopy>> drop_copy_;
  std::vector<std::unique_ptr<WebSocket>> web_socket_;
  std::vector<std::unique_ptr<MarketData>> market_data_;
  std::unique_ptr<Multicast> multicast_;
};

}  // namespace deribit
}  // namespace roq
