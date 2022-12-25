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
  core::Buffer buffer{4096};
  uint64_t msg_seq_num = 0;
  uint64_t processed = 0;
  for (auto _ : state) {
    fix::NewOrderSingle new_order_single{
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
    core::fix::Writer writer{
        buffer,
        core::fix::Version::FIX_44,
        decltype(new_order_single)::msg_type,
        TARGET_COMP_ID,
        SENDER_COMP_ID,
        msg_seq_num,
        SENDING_TIME};
    auto message = new_order_single.encode(writer);
    if (!std::empty(message))
      ++processed;
  }
}

BENCHMARK(BM_fix_new_order_single_create_message);
