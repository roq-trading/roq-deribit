/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <utility>

#include "roq/api.hpp"
#include "roq/server.hpp"

#include "roq/core/memory.hpp"
#include "roq/core/symbols.hpp"

#include "roq/core/limit/rate_limiter.hpp"

#include "roq/core/stack/buffer.hpp"

namespace roq {
namespace deribit {

struct Shared final {
  explicit Shared(server::Dispatcher &);

  Shared(Shared &&) = default;
  Shared(const Shared &) = delete;

  std::string_view next_request_id();

  auto discard_symbol(const std::string_view &name) const {
    return dispatcher_.discard_symbol(name);
  }

  template <typename... Args>
  auto update_order(Args &&...args) {
    return dispatcher_.update_order(std::forward<Args>(args)...);
  }

  template <typename Callback>
  bool find_instrument_name(uint32_t instrument_id, Callback callback) {
    auto iter = instrument_names.find(instrument_id);
    if (iter != instrument_names.end()) {
      callback((*iter).second);
      return true;
    }
    return false;
  }

 public:
  core::page_aligned_vector<Fill> fills;
  core::page_aligned_vector<MBPUpdate> bids, asks, final_bids, final_asks;
  core::page_aligned_vector<Trade> trades;
  core::page_aligned_vector<Statistics> statistics;

  absl::flat_hash_map<Symbol, double> multiplier;

 private:
  server::Dispatcher &dispatcher_;
  uint32_t request_id_ = 0;
  core::stack::Buffer<char, 32> stack_buffer_;

 public:
  core::limit::RateLimiter rate_limiter;
  absl::flat_hash_set<std::string> all_currencies;
  absl::flat_hash_set<Symbol> all_symbols;
  core::Symbols symbols;
  absl::flat_hash_map<uint32_t, Symbol> instrument_names;
};

}  // namespace deribit
}  // namespace roq
