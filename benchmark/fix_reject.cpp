/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=98\00135=3\00149=DERIBITSERVER\00156=ROQ_TRADIN"
  "G\00134=5\00152=20190908-08:47:31.543\00145=5\001372=AN\00158="
  "not_implemented\00110=092\001";
}  // namespace

void BM_fix_reject_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto reject = fix::Reject::parse(message);
          if (!reject.text.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_reject_parse_message);

void BM_fix_parser_dispatch_reject(benchmark::State& state) {
  core::utils::Buffer buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Buffer decode_buffer(buffer);
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          fix::Parser::dispatch(
              overloaded {
                [](const fix::ExecutionReport&) {
                },
                [](const fix::Heartbeat&) {
                },
                [](const fix::Logon&) {
                },
                [](const fix::Logout&) {
                },
                [](const fix::MarketDataIncrementalRefresh&) {
                },
                [](const fix::MarketDataRequestReject&) {
                },
                [](const fix::MarketDataSnapshotFullRefresh&) {
                },
                [](const fix::OrderCancelReject&) {
                },
                [](const fix::PositionReport&) {
                },
                [&](const fix::Reject& reject) {
                  if (!reject.text.empty())
                    ++processed;
                },
                [](const fix::ResendRequest&) {
                },
                [](const fix::SecurityList&) {
                },
                [](const fix::TestRequest&) {
                },
                [](const fix::UserResponse&) {
                },
              },
              message,
              decode_buffer);
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_parser_dispatch_reject);
