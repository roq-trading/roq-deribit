/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/reject.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using Reject = deribit::fix::Reject;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=98\00135=3\00149=DERIBITSERVER\00156=ROQ_TRADIN"
    "G\00134=5\00152=20190908-08:47:31.543\00145=5\001372=AN\00158="
    "not_implemented\00110=092\001"sv;
}  // namespace

void BM_fix_reject_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  auto parser = [&](auto &message_2) {
    auto reject = Reject::create(message_2);
    if (!std::empty(reject.text)) {
      ++processed;
    }
  };
  for (auto _ : state) {
    roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(MESSAGE, parser);
  }
}

BENCHMARK(BM_fix_reject_parse_message);
