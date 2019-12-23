/* Copyright (c) 2017-2020, Hans Erik Thrane */

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
    fix::OrderCancelRequest order_cancel_request = {
      .cl_ord_id = "123",
      .orig_cl_ord_id = "123",
    };
    auto message = order_cancel_request.encode(
        buffer,
        msg_seq_num,
        sending_time);
    if (message.length())
      ++processed;
  }
}

BENCHMARK(BM_fix_order_cancel_request_create_message);
