/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/order_cancel_request.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

// cppcheck-suppress constParameterCallback
void BM_fix_order_cancel_request_create_message(benchmark::State &state) {
  std::vector<std::byte> buffer(4096);
  uint64_t msg_seq_num = 0;
  auto sending_time = 1568702810s;
  for (auto _ : state) {
    auto order_cancel_request = fix::OrderCancelRequest{
        .cl_ord_id = "123"sv,
        .orig_cl_ord_id = "123"sv,
    };
    auto header = core::fix::Header{
        .version = core::fix::Version::FIX_44,
        .msg_type = decltype(order_cancel_request)::msg_type,
        .sender_comp_id = "ROQ_TRADING"sv,
        .target_comp_id = "DERIBITSERVER"sv,
        .msg_seq_num = ++msg_seq_num,  // note!
        .sending_time = sending_time,
    };
    order_cancel_request.encode(header, buffer);
  }
}

BENCHMARK(BM_fix_order_cancel_request_create_message);
