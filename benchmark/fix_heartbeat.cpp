/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=89\00135=0\00149=DERIBITSERVER\00156=ROQ_TRADIN"
  "G\00134=2\00152=20190908-08:47:31.503\001112=anybody in there?"
  "\00110=084\001";
}  // namespace

void BM_fix_heartbeat_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto heartbeat = fix::Heartbeat::parse(message);
          if (heartbeat.test_req_id.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_heartbeat_parse_message);

void BM_fix_parser_dispatch_heartbeat(benchmark::State& state) {
  std::vector<std::byte> buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Buffer decode_buffer(buffer);
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          fix::Parser::dispatch(
              overloaded {
                [](const fix::ExecutionReport&) {
                },
                [&](const fix::Heartbeat& heartbeat) {
                  if (heartbeat.test_req_id.empty())
                    ++processed;
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
                [](const fix::Reject&) {
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

BENCHMARK(BM_fix_parser_dispatch_heartbeat);
