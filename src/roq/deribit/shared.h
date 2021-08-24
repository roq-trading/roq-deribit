/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <utility>

#include "roq/api.h"
#include "roq/server.h"

#include "roq/core/memory.h"

#include "roq/core/stack/buffer.h"

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

 public:
  core::page_aligned_vector<Fill> fills;
  core::page_aligned_vector<MBPUpdate> bids, asks;
  core::page_aligned_vector<Trade> trades;
  core::page_aligned_vector<Statistics> statistics;

  absl::flat_hash_map<std::string, double> multiplier;

 private:
  server::Dispatcher &dispatcher_;
  uint32_t request_id_ = 0;
  core::stack::Buffer<char, 32> stack_buffer_;
};

}  // namespace deribit
}  // namespace roq
