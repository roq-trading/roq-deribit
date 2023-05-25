/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/new_order_single.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

// === CONSTANTS ===
namespace {
auto const REQUEST_ID = "jQAB6gMAAQAAQUIp3sUSAawljiyfnylc"sv;
auto const SYMBOL = "BTC-PERPETUAL"sv;
auto const TARGET_COMP_ID = "ROQ_TRADING"sv;
auto const SENDER_COMP_ID = "DERIBITSERVER"sv;
auto const SENDING_TIME = 1568702810s;
}  // namespace

// === IMPLEMENTATION ===

// cppcheck-suppress constParameterCallback
void BM_fix_new_order_single_create_message(benchmark::State &state) {
  std::vector<std::byte> buffer(4096);
  uint64_t msg_seq_num = 0;
  for (auto _ : state) {
    auto new_order_single = fix::NewOrderSingle{
        .cl_ord_id = REQUEST_ID,
        .side = core::fix::Side::BUY,
        .order_qty = {123.0, Decimals::_0},
        .price = {16833.45, Decimals::_2},
        .symbol = SYMBOL,
        .exec_inst = {},
        .ord_type = core::fix::OrdType::LIMIT,
        .time_in_force = core::fix::TimeInForce::GTC,
        .deribit_label = REQUEST_ID,
        .deribit_adv_order_type = '\0',
    };
    auto header = core::fix::Header{
        .version = core::fix::Version::FIX_44,
        .msg_type = decltype(new_order_single)::msg_type,
        .sender_comp_id = SENDER_COMP_ID,
        .target_comp_id = TARGET_COMP_ID,
        .msg_seq_num = ++msg_seq_num,  // note!
        .sending_time = SENDING_TIME,
    };
    new_order_single.encode(header, buffer);
  }
}

BENCHMARK(BM_fix_new_order_single_create_message);
