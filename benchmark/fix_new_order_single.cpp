/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/new_order_single.h"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

// cppcheck-suppress constParameterCallback
void BM_fix_new_order_single_create_message(benchmark::State &state) {
  core::Buffer buffer(4096);
  uint64_t msg_seq_num = 0;
  auto sending_time = 1568702810s;
  uint64_t processed = 0;
  for (auto _ : state) {
    fix::NewOrderSingle new_order_single = {
        .cl_ord_id = "roq-ord-006"sv,
        .side = core::fix::Side::BUY,
        .order_qty = {2.0, utils::to_decimals(1)},
        .price = {0.45, utils::to_decimals(2)},
        .symbol = "BTC-27SEP19"sv,
        .exec_inst = {},
        .ord_type = core::fix::OrdType::LIMIT,
        .time_in_force = core::fix::TimeInForce::GTC,
        .deribit_label = "roq;123;345"sv,
        .deribit_adv_order_type = '\0',
    };
    core::fix::Writer writer(
        buffer,
        core::fix::Version::FIX_44,
        decltype(new_order_single)::msg_type,
        "ROQ_TRADING"sv,
        "DERIBITSERVER"sv,
        msg_seq_num,
        sending_time);
    auto message = new_order_single.encode(writer);
    if (std::size(message))
      ++processed;
  }
}

BENCHMARK(BM_fix_new_order_single_create_message);
