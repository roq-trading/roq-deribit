/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <memory>

#include "roq/io/sys/signal.hpp"

#include "roq/server.hpp"

#include "roq/deribit/gateway/config.hpp"
#include "roq/deribit/gateway/settings.hpp"

#include "roq/deribit/strategy/settings.hpp"

namespace roq {
namespace deribit {
namespace strategy {

struct Controller final : public server::Handler2, public io::sys::Signal::Handler {
  Controller(gateway::Settings const &, gateway::Config const &, io::Context &context);

  void dispatch();

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

  void refresh(std::chrono::nanoseconds now);

  void create_order();
  void cancel_order();

  // server::Handler2

  void operator()(Trace<HandshakeAck> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<Control> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<DownloadBegin> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<DownloadEnd> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<Ready> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<GatewaySettings> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<StreamStatus> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<GatewayStatus> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<ExternalLatency> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<RateLimitsUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<RateLimitTrigger> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<ReferenceData> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<MarketStatus> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<TopOfBook> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<MarketByPriceUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<MarketByOrderUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<TradeSummary> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<StatisticsUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<TimeSeriesUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<CancelAllOrdersAck> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<OrderAck> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<OrderUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<TradeUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<PositionUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;
  void operator()(Trace<FundsUpdate> const &, uint64_t opaque, bool is_last, uint8_t user_id) override;

  // io::sys::Signal::Handler

  void operator()(io::sys::Signal::Event const &) override;

 private:
  std::unique_ptr<io::sys::Signal> const terminate_;
  std::unique_ptr<io::sys::Signal> const interrupt_;
  std::unique_ptr<server::Strategy> const dispatcher_;
  State state_ = {};
  std::chrono::nanoseconds next_update_ = {};
  uint64_t max_order_id_ = {};
  uint64_t order_id_ = {};
};

}  // namespace strategy
}  // namespace deribit
}  // namespace roq
