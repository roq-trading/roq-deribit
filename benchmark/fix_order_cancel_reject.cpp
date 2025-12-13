/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/order_cancel_reject.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using OrderCancelReject = deribit::fix::OrderCancelReject;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=99\00135=9\00149=DERIBITSERVER\00156=ROQ_TRADIN"
    "G\00134=3\00152=20190908-17:39:23.573\00141=123\00111=345\0013"
    "9=8\00158=not_found\00110=000\001"sv;
}  // namespace

void BM_fix_order_cancel_reject_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  auto parser = [&](auto &message_2) {
    auto result = OrderCancelReject::create(message_2);
    if (!std::empty(result.text)) {
      ++processed;
    }
  };
  for (auto _ : state) {
    roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(MESSAGE, parser);
  }
}

BENCHMARK(BM_fix_order_cancel_reject_parse_message);
