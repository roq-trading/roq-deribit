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
    fix::NewOrderSingle new_order_single = {
      .cl_ord_id = "roq-ord-006",
      .side = core::fix::Side::BUY,
      .order_qty = 2.0,
      .price = 0.5,
      .symbol = "BTC-27SEP19",
      .ord_type = core::fix::OrdType::LIMIT,
      .time_in_force = core::fix::TimeInForce::GTC,
      .deribit_label = "roq;123;345",
    };
    auto message = new_order_single.encode(
        buffer,
        msg_seq_num,
        sending_time);
    if (message.length())
      ++processed;
  }
}

BENCHMARK(BM_fix_new_order_single_create_message);
