/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/market_data_request_reject.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using MarketDataRequestReject = deribit::fix::MarketDataRequestReject;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=102\00135=Y\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=4\00152=20190908-10:54:45.738\001262=123\00158=unknow"
    "n Symbol: BTC-XXX\00110=152\001"sv;
}  // namespace

void BM_fix_market_data_request_reject_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  auto parser = [&](auto &message_2) {
    auto market_data_request_reject = MarketDataRequestReject::create(message_2);
    if (!std::empty(market_data_request_reject.text)) {
      ++processed;
    }
  };
  for (auto _ : state) {
    roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(MESSAGE, parser);
  }
}

BENCHMARK(BM_fix_market_data_request_reject_parse_message);
