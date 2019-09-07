/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
    "8=FIX.4.4\0019=68\00135=0\00149=DERIBITSERVER\00156=ROQ_TRADIN"
    "G\00134=22\00152=20190907-16:46:08.285\00110=085\001";
}  // namespace

void BM_fix_heartbeat_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    auto bytes = core::fix::Reader::dispatch(
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
    auto bytes = core::fix::Reader::dispatch(
        [&](const core::fix::message_t& message) {
          fix::Parser::dispatch(
              overloaded {
                [](const fix::ExecutionReport& execution_report) {
                },
                [&](const fix::Heartbeat& heartbeat) {
                  if (heartbeat.test_req_id.empty())
                    ++processed;
                },
                [](const fix::Logon& logon) {
                },
                [](const fix::Logout& logout) {
                },
                [](const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
                },
                [](const fix::MarketDataRequestReject& market_data_request_reject) {
                },
                [](const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
                },
                [](const fix::PositionReport& position_report) {
                },
                [](const fix::ResendRequest& resend_request) {
                },
                [](const fix::SecurityList& security_list) {
                },
                [](const fix::TestRequest& test_request) {
                },
              },
              message,
              buffer);
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_parser_dispatch_heartbeat);
