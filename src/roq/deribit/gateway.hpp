/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <vector>

#include "roq/server.hpp"

#include "roq/io/context.hpp"

#include "roq/deribit/authenticator.hpp"
#include "roq/deribit/config.hpp"
#include "roq/deribit/drop_copy.hpp"
#include "roq/deribit/market_data.hpp"
#include "roq/deribit/order_entry.hpp"
#include "roq/deribit/shared.hpp"
#include "roq/deribit/udp_events.hpp"
#include "roq/deribit/udp_snapshot.hpp"
#include "roq/deribit/web_socket.hpp"

namespace roq {
namespace deribit {

struct Gateway final : public server::Handler,
                       public OrderEntry::Handler,
                       public DropCopy::Handler,
                       public WebSocket::Handler,
                       public MarketData::Handler,
                       public UDPSnapshot::Handler,
                       public UDPEvents::Handler {
  Gateway(server::Dispatcher &, Config const &, io::Context &);

  Gateway(Gateway &&) = delete;
  Gateway(Gateway const &) = delete;

 protected:
  // server::Handler

  void operator()(Event<Start> const &) override;
  void operator()(Event<Stop> const &) override;
  void operator()(Event<Timer> const &) override;
  void operator()(Event<server::Refresh> const &) override;
  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;

  uint16_t operator()(Event<CreateOrder> const &, oms::Order const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Event<ModifyOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Event<CancelOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id) override;

  void operator()(metrics::Writer &) override;

  // many

  void operator()(Trace<StreamStatus> const &) override;
  void operator()(Trace<ExternalLatency> const &) override;
  void operator()(Trace<ReferenceData> const &, bool is_last) override;
  void operator()(Trace<MarketStatus> const &, bool is_last) override;
  void operator()(Trace<TopOfBook> const &, bool is_last) override;
  void operator()(Trace<MarketByPriceUpdate> const &, bool is_last) override;
  void operator()(Trace<TradeSummary> const &, bool is_last) override;
  void operator()(Trace<StatisticsUpdate> const &, bool is_last) override;
  void operator()(Trace<oms::TradeUpdate> const &, uint16_t stream_id, bool is_last, uint8_t user_id) override;
  void operator()(Trace<PositionUpdate> const &, bool is_last) override;
  void operator()(Trace<FundsUpdate> const &, bool is_last) override;

  void operator()(WebSocket::CurrenciesUpdate &) override;
  void operator()(WebSocket::SymbolsUpdate &) override;

  void operator()(MarketData::SymbolsUpdate &) override;

  void ensure_symbol_slices(size_t size);

  // utilities

  template <typename... Args>
  void dispatch(Args &&...);

  OrderEntry &get_order_entry(std::string_view const &account);

 private:
  server::Dispatcher &dispatcher_;
  // config
  const std::string master_account_;
  // authentication
  absl::flat_hash_map<Account, std::unique_ptr<Authenticator>> authenticator_;
  // io
  io::Context &context_;
  // shared
  Shared shared_;
  // seed
  uint16_t stream_id_ = {};
  // streams
  absl::flat_hash_map<Account, std::unique_ptr<OrderEntry>> order_entry_;
  absl::flat_hash_map<Account, std::unique_ptr<DropCopy>> drop_copy_;
  std::vector<std::unique_ptr<WebSocket>> web_socket_;
  std::vector<std::unique_ptr<MarketData>> market_data_;
  std::unique_ptr<UDPSnapshot> udp_snapshot_;
  std::unique_ptr<UDPEvents> udp_events_;
  // cache
  std::vector<MBPUpdate> bids_, asks_;
};

}  // namespace deribit
}  // namespace roq
