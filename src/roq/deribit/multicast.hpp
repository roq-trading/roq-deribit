/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/net/udp_connection.hpp"

#include "roq/server.hpp"

#include "roq/deribit/shared.hpp"

namespace roq {
namespace deribit {

class Multicast final : public core::net::UdpConnection::Handler {
 public:
  struct Handler {
    virtual void operator()(const Trace<StreamStatus> &) = 0;
    virtual void operator()(const Trace<TopOfBook> &, bool is_last) = 0;
    virtual void operator()(const Trace<MarketByPriceUpdate> &, bool is_last, bool refresh) = 0;
    virtual void operator()(const Trace<TradeSummary> &, bool is_last) = 0;
  };

  Multicast(Handler &, core::io::Context &, uint16_t stream_id, Shared &);

  Multicast(const Multicast &) = delete;
  Multicast(Multicast &&) = delete;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

 protected:
  void operator()(const core::net::UdpConnection::Read &) override;

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  // connection
  core::net::UdpConnection events_;
  core::net::UdpConnection snapshot_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse;
  } profile_;
  // cache
  Shared &shared_;
};

}  // namespace deribit
}  // namespace roq
