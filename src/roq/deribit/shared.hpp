/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <absl/container/node_hash_map.h>

#include <string>
#include <utility>
#include <vector>

#include "roq/api.hpp"
#include "roq/server.hpp"

#include "roq/core/symbols.hpp"

#include "roq/core/limit/rate_limiter.hpp"

#include "roq/core/stack/buffer.hpp"

#include "roq/core/mbp/sequencer.hpp"

#include "roq/deribit/instrument.hpp"

namespace roq {
namespace deribit {

struct Shared final {
  explicit Shared(server::Dispatcher &);

  Shared(Shared &&) = default;
  Shared(Shared const &) = delete;

  bool has_multicast() const { return multicast_; }

  std::string_view next_request_id();

  auto get_user_name(uint8_t user_id) const { return dispatcher_.get_user_name(user_id); }

  auto discard_symbol(std::string_view const &name) const { return dispatcher_.discard_symbol(name); }

  template <typename... Args>
  auto update_order(Args &&...args) {
    return dispatcher_.update_order(std::forward<Args>(args)...);
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
    return dispatcher_(std::forward<Args>(args)...);
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

  absl::flat_hash_map<Symbol, double> multiplier;

 private:
  server::Dispatcher &dispatcher_;
  uint32_t request_id_ = 0;
  core::stack::Buffer<char, 32> stack_buffer_;
  bool const multicast_;

 public:
  core::limit::RateLimiter rate_limiter;
  absl::flat_hash_set<std::string> all_currencies;
  absl::flat_hash_set<Symbol> all_symbols;
  core::Symbols symbols;
  absl::node_hash_map<uint32_t, std::pair<Instrument, bool>> instruments;
  absl::node_hash_map<Symbol, core::mbp::Sequencer> mbp_sequencer;
};

}  // namespace deribit
}  // namespace roq
