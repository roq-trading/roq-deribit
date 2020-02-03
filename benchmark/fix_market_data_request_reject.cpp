/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/market_data_request_reject.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=102\00135=Y\00149=DERIBITSERVER\00156=ROQ_TRADI"
  "NG\00134=4\00152=20190908-10:54:45.738\001262=123\00158=unknow"
  "n Symbol: BTC-XXX\00110=152\001";
}  // namespace

void BM_fix_market_data_request_reject_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto market_data_request_reject = fix::MarketDataRequestReject::create(message);
          if (!market_data_request_reject.text.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_market_data_request_reject_parse_message);
