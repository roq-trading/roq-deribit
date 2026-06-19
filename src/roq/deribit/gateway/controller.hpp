/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>
#include <string>
#include <vector>

#include "roq/server.hpp"

#include "roq/utils/container.hpp"

#include "roq/io/context.hpp"

#include "roq/deribit/gateway/account.hpp"
#include "roq/deribit/gateway/config.hpp"
#include "roq/deribit/gateway/request.hpp"
#include "roq/deribit/gateway/settings.hpp"
#include "roq/deribit/gateway/shared.hpp"

#include "roq/deribit/gateway/drop_copy.hpp"
#include "roq/deribit/gateway/market_data.hpp"
#include "roq/deribit/gateway/order_entry.hpp"
#include "roq/deribit/gateway/rest.hpp"
#include "roq/deribit/gateway/udp_events.hpp"
#include "roq/deribit/gateway/udp_snapshot.hpp"
#include "roq/deribit/gateway/web_socket.hpp"

namespace roq {
namespace deribit {
namespace gateway {

struct Controller final : public server::Handler,
                          public Rest::Handler,
                          public OrderEntry::Handler,
                          public DropCopy::Handler,
                          public WebSocket::Handler,
                          public MarketData::Handler,
                          public UDPSnapshot::Handler,
                          public UDPEvents::Handler {
  ROQ_PUBLIC static std::unique_ptr<server::Handler> create(server::Dispatcher &, Settings const &, Config const &, io::Context &);

  ROQ_PUBLIC static uint8_t parse_api(Settings const &);

  Controller(server::Dispatcher &, Settings const &, Config const &, io::Context &);

  Controller(Controller const &) = delete;

 protected:
  // server::Handler

  void operator()(Event<Start> const &) override;
  void operator()(Event<Stop> const &) override;
  void operator()(Event<Timer> const &) override;
  void operator()(Event<Control> const &) override;
  void operator()(Event<server::Refresh> const &) override;
  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;

  void operator()(Event<Subscribe> const &) override;

  uint16_t operator()(Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Event<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Event<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id) override;

  uint16_t operator()(Event<MassQuote> const &) override;

  uint16_t operator()(Event<CancelQuotes> const &) override;

  void operator()(metrics::Writer &) const override;

  // Rest::Handler

  void operator()(Rest::CurrenciesUpdate &) override;
  void operator()(Rest::SymbolsUpdate &) override;

  // WebSocket::Handler

  void operator()(WebSocket::Latch const &) override;

  // MarketData::Handler

  void operator()(MarketData::SymbolsUpdate &) override;

  // utilities

  void ensure_symbol_slices(size_t size);

  template <typename... Args>
  void dispatch(Args &&...);

  template <typename... Args>
  static void dispatch_helper(auto &self, Args &&...);

  Account &get_account(std::string_view const &account);

  OrderEntry &get_order_entry(std::string_view const &account);

 private:
  server::Dispatcher &dispatcher_;
  // config
  std::string const master_account_;
  // accounts
  utils::unordered_map<std::string, std::unique_ptr<Account>> accounts_;
  // io
  io::Context &context_;
  // shared
  Shared shared_;
  // seed
  uint16_t stream_id_ = {};
  //
  Request request_;
  // streams
  Rest rest_;
  utils::unordered_map<std::string, std::unique_ptr<OrderEntry>> order_entry_;
  utils::unordered_map<std::string, std::unique_ptr<DropCopy>> drop_copy_;
  std::vector<std::unique_ptr<WebSocket>> web_socket_;
  std::vector<std::unique_ptr<MarketData>> market_data_;
  std::unique_ptr<UDPSnapshot> udp_snapshot_;
  std::unique_ptr<UDPEvents> udp_events_;
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
