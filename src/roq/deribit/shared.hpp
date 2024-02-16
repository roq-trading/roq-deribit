/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "roq/api.hpp"
#include "roq/server.hpp"

#include "roq/utils/container.hpp"

#include "roq/market/mbp/sequencer.hpp"

#include "roq/core/symbols.hpp"

#include "roq/core/limit/rate_limiter.hpp"

#include "roq/deribit/instrument.hpp"
#include "roq/deribit/settings.hpp"

namespace roq {
namespace deribit {

struct Shared final {
  Shared(server::Dispatcher &, Settings const &);

  Shared(Shared &&) = default;
  Shared(Shared const &) = delete;

  bool has_multicast() const { return multicast_; }

  std::string_view next_request_id();

  auto discard_symbol(std::string_view const &name) const { return dispatcher.discard_symbol(name); }

  template <typename... Args>
  auto find_order(Args &&...args) {
    return dispatcher.find_order(std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto update_order(Args &&...args) {
    return dispatcher.update_order(std::forward<Args>(args)...);
  }

  template <typename Callback>
  bool find_instrument(uint32_t instrument_id, Callback callback) {
    auto iter = instruments.find(instrument_id);
    if (iter != std::end(instruments)) {
      auto &[symbol, discard] = (*iter).second;
      if (!discard)
        callback((*iter).second.first);
      return true;
    }
    return false;
  }

  template <typename Callback>
  std::pair<Instrument const &, bool> find_instrument_name_with_create(uint32_t instrument_id, Callback callback) {
    auto iter = instruments.find(instrument_id);
    if (iter == std::end(instruments)) {
      auto instrument = callback();
      auto discard = discard_symbol(instrument.symbol);
      auto res = instruments.try_emplace(instrument_id, instrument, discard);
      assert(res.second);
      iter = res.first;
    }
    return {(*iter).second.first, (*iter).second.second};
  }

  template <typename... Args>
  auto operator()(Args &&...args) {
    return dispatcher(std::forward<Args>(args)...);
  }

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

  auto &get_trades() {
    trades.clear();
    return trades;
  }

  auto &get_statistics() {
    statistics.clear();
    return statistics;
  }

  utils::unordered_map<std::string, double> multiplier;

  auto &get_mbp_sequencer(std::string_view const &symbol) {
    auto iter = mbp_sequencer.find(symbol);
    if (iter != std::end(mbp_sequencer)) {
      market::mbp::Sequencer sequencer;
      auto res = mbp_sequencer.try_emplace(symbol, std::move(sequencer));
      assert(res.second);
      iter = res.first;
    }
    return (*iter).second;
  }

 public:
  server::Dispatcher &dispatcher;
  Settings const &settings;

 private:
  uint32_t request_id_ = 0;
  std::string request_id_encode_buffer_;
  bool const multicast_;

 public:
  core::limit::RateLimiter rate_limiter;
  utils::unordered_set<std::string> all_currencies;
  utils::unordered_set<std::string> all_symbols;
  core::Symbols symbols;
  utils::unordered_map<uint32_t, std::pair<Instrument, bool>> instruments;
  utils::unordered_map<std::string, market::mbp::Sequencer> mbp_sequencer;
};

}  // namespace deribit
}  // namespace roq
