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

 protected:
  enum class State {
    READY,
    CREATE_ORDER,
    WAITING_CREATE,
    WORKING,
    CANCEL_ORDER,
    WAITING_CANCEL,
    DONE,
  };

  void operator()(State);

  void refresh(std::chrono::nanoseconds now);

  void create_order();
  void cancel_order();

  // server::Strategy::Handler
  // XXX HANS => Event
  void operator()(Trace<DownloadBegin> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<DownloadEnd> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<Ready> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<GatewaySettings> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<StreamStatus> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<GatewayStatus> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<ReferenceData> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<MarketStatus> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<TopOfBook> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<MarketByPriceUpdate> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<OrderAck> const &, uint64_t opaque, bool is_last) override;
  void operator()(Trace<OrderUpdate> const &, uint64_t opaque, bool is_last) override;

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
