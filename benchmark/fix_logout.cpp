/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/logout.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
    "8=FIX.4.4\0019=90\00135=5\00149=DERIBITSERVER\00156=ROQ_TRADIN"
    "G\00134=1\00152=20190907-16:56:43.398\00158=invalid_credential"
    "s\00110=166\001";
}  // namespace

// cppcheck-suppress constParameterCallback
void BM_fix_logout_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t &message) {
          auto logout = fix::Logout::create(message);
          if (logout.text > 0) ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_logout_parse_message);
