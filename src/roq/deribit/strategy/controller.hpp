/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <span>
#include <string_view>

#include "roq/api.hpp"

#include "roq/io/sys/signal.hpp"

#include "roq/server.hpp"

#include "roq/deribit/gateway/config.hpp"
#include "roq/deribit/gateway/settings.hpp"

#include "roq/deribit/strategy/settings.hpp"

namespace roq {
namespace deribit {
namespace strategy {

struct Controller final : public server::Strategy::Handler, public server::Strategy, public io::sys::Signal::Handler {
  Controller(gateway::Settings const &, gateway::Config const &, io::Context &context);

  void refresh(std::chrono::nanoseconds now);

 protected:
  enum class State {
    UNDEFINED,
    READY,
    CREATE_ORDER,
    WAITING_CREATE,
    WORKING,
    CANCEL_ORDER,
    WAITING_CANCEL,
    DONE,
  };

  void operator()(State);

  void create_order();
  void cancel_order();

  // server::Strategy::Handler
  void operator()(Event<DownloadBegin> const &) override;
  void operator()(Event<DownloadEnd> const &) override;
  void operator()(Event<Ready> const &) override;
  void operator()(Event<GatewaySettings> const &) override;
  void operator()(Event<StreamStatus> const &) override;
  void operator()(Event<GatewayStatus> const &) override;
  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;
  void operator()(Event<TopOfBook> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<OrderAck> const &) override;
  void operator()(Event<OrderUpdate> const &) override;

  // io::sys::Signal::Handler
  void operator()(io::sys::Signal::Event const &) override;

 private:
  Settings const &settings_;
  std::unique_ptr<io::sys::Signal> terminate_;
  std::unique_ptr<io::sys::Signal> interrupt_;
  State state_ = {};
  std::chrono::nanoseconds next_update_ = {};
  uint64_t max_order_id_ = {};
  uint64_t order_id_ = {};
};

}  // namespace strategy
}  // namespace deribit
}  // namespace roq
