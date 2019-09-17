/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/order_cancel_replace_request.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

void BM_fix_order_cancel_replace_request_create_message(benchmark::State& state) {
  core::utils::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  uint64_t processed = 0;
  for (auto _ : state) {
    auto message = fix::OrderCancelReplaceRequest::encode(
        buffer,
        msg_seq_num,
        sending_time,
        "123",
        "123",
        core::fix::Side::BUY,
        1.0,
        core::fix::OrdType::LIMIT,
        1.0,
        "BTC-27SEP19",
        sending_time);
    if (message.length())
      ++processed;
  }
}

BENCHMARK(BM_fix_order_cancel_replace_request_create_message);
