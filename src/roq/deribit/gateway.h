/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <list>
#include <memory>
#include <string>

#include "roq/server.h"

#include "roq/core/io/context.h"

#include "roq/deribit/config.h"
#include "roq/deribit/market_data.h"
#include "roq/deribit/order_entry.h"
#include "roq/deribit/security.h"
#include "roq/deribit/shared.h"
#include "roq/deribit/web_socket.h"

namespace roq {
namespace deribit {

class Gateway final : public server::Handler,
                      public WebSocket::Handler,
                      public OrderEntry::Handler,
                      public MarketData::Handler {
 public:
  Gateway(server::Dispatcher &, const Config &);

  Gateway(Gateway &&) = delete;
  Gateway(const Gateway &) = delete;

 protected:
  // server::Handler

  void operator()(const Event<Start> &) override;
  void operator()(const Event<Stop> &) override;
  void operator()(const Event<Timer> &) override;
  void operator()(const Event<Connection> &) override;

  void operator()(
      const Event<CreateOrder> &,
      const std::string_view &request_id,
      uint32_t gateway_order_id) override;
  void operator()(
      const Event<ModifyOrder> &,
      const std::string_view &request_id,
      const server::OMS_Order &) override;
  void operator()(
      const Event<CancelOrder> &,
      const std::string_view &request_id,
      const server::OMS_Order &) override;

  void operator()(metrics::Writer &) override;

  // many

  void operator()(const server::Trace<ExternalLatency> &) override;

  void operator()(const server::Trace<MarketDataStatus> &) override;
  void operator()(const server::Trace<ReferenceData> &, bool is_last) override;
  void operator()(const server::Trace<MarketStatus> &, bool is_last) override;
  void operator()(const server::Trace<TopOfBook> &, bool is_last) override;
  void operator()(const server::Trace<MarketByPriceUpdate> &, bool is_last) override;
  void operator()(const server::Trace<TradeSummary> &, bool is_last) override;
  void operator()(const server::Trace<StatisticsUpdate> &, bool is_last) override;

  void operator()(const server::Trace<OrderManagerStatus> &) override;
  void operator()(const server::Trace<OrderAck> &, bool is_last, uint8_t user_id) override;
  void operator()(const server::Trace<OrderUpdate> &, bool is_last, uint8_t user_id) override;
  void operator()(const server::Trace<TradeUpdate> &, bool is_last, uint8_t user_id) override;
  void operator()(const server::Trace<PositionUpdate> &, bool is_last) override;
  void operator()(const server::Trace<FundsUpdate> &, bool is_last) override;

  void operator()(MarketData::SymbolsUpdate &) override;

  // utilities

  OrderEntry &get_order_entry(const std::string_view &account);

 private:
  server::Dispatcher &dispatcher_;
  // config
  const std::string master_account_;
  // security
  absl::flat_hash_map<std::string, std::unique_ptr<Security>> security_;
  // io
  core::io::Context context_;
  // shared
  Shared shared_;
  // seed
  uint16_t stream_id_ = {};
  // streams
  WebSocket web_socket_;
  absl::flat_hash_map<std::string, std::unique_ptr<OrderEntry>> order_entry_;
  std::list<std::unique_ptr<MarketData>> market_data_;
};

}  // namespace deribit
}  // namespace roq
