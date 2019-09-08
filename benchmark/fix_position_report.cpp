/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=106\00135=AP\00149=DERIBITSERVER\00156=ROQ_TRAD"
  "ING\00134=4\00152=20190908-08:47:31.543\001721=2950652\001710="
  "123\001724=0\001728=0\001702=0\00110=125\001";
}  // namespace

void BM_fix_position_report_parse_message(benchmark::State& state) {
  std::vector<std::byte> buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    auto bytes = core::fix::Reader::dispatch(
        [&](const core::fix::message_t& message) {
          auto position_report = fix::PositionReport::parse(message, buffer);
          if (!position_report.pos_req_id.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_position_report_parse_message);

void BM_fix_parser_dispatch_position_report(benchmark::State& state) {
  std::vector<std::byte> buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    auto bytes = core::fix::Reader::dispatch(
        [&](const core::fix::message_t& message) {
          fix::Parser::dispatch(
              overloaded {
                [](const fix::ExecutionReport& execution_report) {
                },
                [](const fix::Heartbeat& heartbeat) {
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
                [&](const fix::PositionReport& position_report) {
                  if (!position_report.pos_req_id.empty())
                    ++processed;
                },
                [](const fix::Reject& reject) {
                },
                [](const fix::ResendRequest& resend_request) {
                },
                [](const fix::SecurityList& security_list) {
                },
                [](const fix::TestRequest& test_request) {
                },
                [](const fix::UserResponse& user_response) {
                },
              },
              message,
              buffer);
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_parser_dispatch_position_report);
