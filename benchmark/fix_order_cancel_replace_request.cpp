/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/order_cancel_replace_request.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using OrderCancelReplaceRequest = deribit::fix::OrderCancelReplaceRequest;

void BM_fix_order_cancel_replace_request_create_message(benchmark::State &state) {
  std::vector<std::byte> buffer(4096);
  uint64_t msg_seq_num = 0;
  auto sending_time = 1568702810s;
  for (auto _ : state) {
    auto order_cancel_replace_request = OrderCancelReplaceRequest{
        .cl_ord_id = "123"sv,
        .orig_cl_ord_id = "123"sv,
        .deribit_label = "123"sv,
        .symbol = "BTC-27SEP19"sv,
        .currency = {},
        .side = roq::fix::Side::BUY,
        .order_qty = {1.0, Precision::_1},
        .ord_type = roq::fix::OrdType::LIMIT,
        .price = {123.45, Precision::_2},
        .exec_inst = {},
        .deribit_mm_protection = {},
    };
    auto header = roq::fix::Header{
        .version = roq::fix::Version::FIX_44,
        .msg_type = decltype(order_cancel_replace_request)::MSG_TYPE,
        .sender_comp_id = "ROQ_TRADING"sv,
        .target_comp_id = "DERIBITSERVER"sv,
        .msg_seq_num = ++msg_seq_num,  // note!
        .sending_time = sending_time,
    };
    order_cancel_replace_request.encode(header, buffer);
  }
}

BENCHMARK(BM_fix_order_cancel_replace_request_create_message);
