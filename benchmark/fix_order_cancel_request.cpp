/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/order_cancel_request.h"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

// cppcheck-suppress constParameterCallback
void BM_fix_order_cancel_request_create_message(benchmark::State &state) {
  core::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  uint64_t processed = 0;
  for (auto _ : state) {
    fix::OrderCancelRequest order_cancel_request = {
        .cl_ord_id = "123"sv,
        .orig_cl_ord_id = "123"sv,
    };
    core::fix::Writer writer(
        buffer,
        core::fix::Version::FIX_44,
        decltype(order_cancel_request)::msg_type,
        "ROQ_TRADING"sv,
        "DERIBITSERVER"sv,
        msg_seq_num,
        sending_time);
    auto message = order_cancel_request.encode(writer);
    if (std::size(message))
      ++processed;
  }
}

BENCHMARK(BM_fix_order_cancel_request_create_message);
