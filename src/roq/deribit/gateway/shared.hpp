/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "roq/api.hpp"
#include "roq/server.hpp"

#include "roq/utils/container.hpp"

#include "roq/core/symbols.hpp"
#include "roq/core/timer_queue.hpp"

#include "roq/core/limit/rate_limiter.hpp"

#include "roq/deribit/protocol/sbe/config.hpp"

#include "roq/deribit/gateway/api.hpp"
#include "roq/deribit/gateway/instrument.hpp"
#include "roq/deribit/gateway/settings.hpp"

namespace roq {
namespace deribit {
namespace gateway {

struct Shared final {
  Shared(server::Dispatcher &, Settings const &);

  Shared(Shared const &) = delete;

  bool has_multicast() const { return multicast_; }

  std::string_view next_request_id();

  template <typename Callback>
  bool find_instrument(uint32_t instrument_id, Callback callback) {
    auto iter = instruments.find(instrument_id);
    if (iter != std::end(instruments)) {
      auto &instrument = (*iter).second;
      if (!instrument.discard) {
        callback(instrument);
      }
      return true;
    }
    return false;
  }

  template <typename Callback>
  void maybe_create_instrument(uint32_t instrument_id, Callback callback) {
    auto iter = instruments.find(instrument_id);
    if (iter != std::end(instruments)) {
      return;
    }
    auto instrument = callback();
    [[maybe_unused]] auto res = instruments.try_emplace(instrument_id, std::move(instrument));
    assert(res.second);
  }

  server::Dispatcher &dispatcher;

  Settings const &settings;
  API const api;

  core::limit::RateLimiter rate_limiter;

  core::Symbols symbols;
  utils::unordered_set<std::string> all_currencies;
  utils::unordered_set<std::string> all_symbols;

 private:
  std::vector<Fill> fills;
  struct {
    std::vector<MBPUpdate> bids, asks;
    auto &clear() {
      bids.clear();
      asks.clear();
      return *this;
    }
    bool empty() const { return std::empty(bids) && std::empty(asks); }
  } mbp;
  std::vector<Trade> trades;
  std::vector<Statistics> statistics;

 public:
  auto &get_fills() {
    fills.clear();
    return fills;
  }

  auto &get_mbp() { return mbp.clear(); }

  std::vector<MBPUpdate> final_bids, final_asks;

  auto &get_trades() {
    trades.clear();
    return trades;
  }

  auto &get_statistics() {
    statistics.clear();
    return statistics;
  }

  utils::unordered_map<std::string, double> multiplier;

 private:
  uint32_t request_id_ = 0;
  std::string request_id_encode_buffer_;
  bool const multicast_;

 public:
  utils::unordered_map<uint32_t, Instrument> instruments;

  std::vector<std::byte> buffer;

  protocol::sbe::Config const sbe_config;

  std::vector<TickSizeStep> tick_size_steps;

  core::TimerQueue<std::string> time_series_request_queue;

  std::vector<Bar> bars;
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
