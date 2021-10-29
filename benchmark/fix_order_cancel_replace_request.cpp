/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/order_cancel_replace_request.h"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

void BM_fix_order_cancel_replace_request_create_message(
    benchmark::State &state) {  // cppcheck-suppress constParameterCallback
  core::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  uint64_t processed = 0;
  for (auto _ : state) {
    fix::OrderCancelReplaceRequest order_cancel_replace_request = {
        .orig_cl_ord_id = "123"sv,
        .cl_ord_id = "123"sv,
        .transact_time = sending_time,
        .side = core::fix::Side::BUY,
        .order_qty = {1.0, 1},
        .ord_type = core::fix::OrdType::LIMIT,
        .price = {123.45, 2},
        .symbol = "BTC-27SEP19"sv,
        .exec_inst = {},
    };
    core::fix::Writer writer(
        buffer,
        core::fix::Version::FIX_44,
        decltype(order_cancel_replace_request)::msg_type,
        "ROQ_TRADING"sv,
        "DERIBITSERVER"sv,
        msg_seq_num,
        sending_time);
    auto message = order_cancel_replace_request.encode(writer);
    if (std::size(message))
      ++processed;
  }
}

BENCHMARK(BM_fix_order_cancel_replace_request_create_message);
