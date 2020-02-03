/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/order_cancel_reject.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=99\00135=9\00149=DERIBITSERVER\00156=ROQ_TRADIN"
  "G\00134=3\00152=20190908-17:39:23.573\00141=123\00111=345\0013"
  "9=8\00158=not_found\00110=000\001";
}  // namespace

void BM_fix_order_cancel_reject_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto result = fix::OrderCancelReject::create(message);
          if (!result.text.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_order_cancel_reject_parse_message);
