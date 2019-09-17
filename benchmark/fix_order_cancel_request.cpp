/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/order_cancel_request.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

void BM_fix_order_cancel_request_create_message(benchmark::State& state) {
  core::utils::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  uint64_t processed = 0;
  for (auto _ : state) {
    auto message = fix::OrderCancelRequest::encode(
        buffer,
        msg_seq_num,
        sending_time,
        "123",
        "123");
    if (message.length())
      ++processed;
  }
}

BENCHMARK(BM_fix_order_cancel_request_create_message);
