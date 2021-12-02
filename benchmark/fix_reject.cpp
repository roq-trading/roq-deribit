/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/reject.h"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

namespace {
const auto MESSAGE =
    "8=FIX.4.4\0019=98\00135=3\00149=DERIBITSERVER\00156=ROQ_TRADIN"
    "G\00134=5\00152=20190908-08:47:31.543\00145=5\001372=AN\00158="
    "not_implemented\00110=092\001"sv;
}  // namespace

// cppcheck-suppress constParameterCallback
void BM_fix_reject_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t &message) {
          auto reject = fix::Reject::create(message);
          if (!std::empty(reject.text))
            ++processed;
        },
        MESSAGE);
  }
}

BENCHMARK(BM_fix_reject_parse_message);
