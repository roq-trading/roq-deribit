/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/new_order_single.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

void BM_fix_new_order_single_create_message(benchmark::State& state) {
  core::utils::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  uint64_t processed = 0;
  for (auto _ : state) {
    auto message = fix::NewOrderSingle::encode(
        buffer,
        msg_seq_num,
        sending_time,
        "roq-ord-006",
        core::fix::Side::BUY,
        2.0,
        0.5,
        "BTC-27SEP19",
        core::fix::OrdType::LIMIT,
        core::fix::TimeInForce::GTC,
        "roq;123;345");
    if (message.length())
      ++processed;
  }
}

BENCHMARK(BM_fix_new_order_single_create_message);
