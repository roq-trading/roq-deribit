/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/new_order_single.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using NewOrderSingle = deribit::fix::NewOrderSingle;

// === CONSTANTS ===
namespace {
auto const REQUEST_ID = "jQAB6gMAAQAAQUIp3sUSAawljiyfnylc"sv;
auto const SYMBOL = "BTC-PERPETUAL"sv;
auto const TARGET_COMP_ID = "ROQ_TRADING"sv;
auto const SENDER_COMP_ID = "DERIBITSERVER"sv;
auto const SENDING_TIME = 1568702810s;
}  // namespace

// === IMPLEMENTATION ===

void BM_fix_new_order_single_create_message(benchmark::State &state) {
  std::vector<std::byte> buffer(4096);
  uint64_t msg_seq_num = 0;
  for (auto _ : state) {
    auto new_order_single = NewOrderSingle{
        .cl_ord_id = REQUEST_ID,
        .side = roq::fix::Side::BUY,
        .order_qty = {123.0, Precision::_0},
        .price = {16833.45, Precision::_2},
        .symbol = SYMBOL,
        .exec_inst = {},
        .ord_type = roq::fix::OrdType::LIMIT,
        .time_in_force = roq::fix::TimeInForce::GTC,
        .stop_px = {},
        .deribit_label = REQUEST_ID,
        .deribit_adv_order_type = '\0',
        .deribit_mm_protection = {},
        .deribit_condition_trigger_method = {},
    };
    auto header = roq::fix::Header{
        .version = roq::fix::Version::FIX_44,
        .msg_type = decltype(new_order_single)::MSG_TYPE,
        .sender_comp_id = SENDER_COMP_ID,
        .target_comp_id = TARGET_COMP_ID,
        .msg_seq_num = ++msg_seq_num,  // note!
        .sending_time = SENDING_TIME,
    };
    new_order_single.encode(header, buffer);
  }
}

BENCHMARK(BM_fix_new_order_single_create_message);
