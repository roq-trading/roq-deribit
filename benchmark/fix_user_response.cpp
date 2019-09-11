/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=199\00135=BF\00149=DERIBITSERVER\00156=ROQ_TRAD"
  "ING\00134=3\00152=20190908-08:47:31.511\001923=123\001553=5MP4"
  "0u9h\001926=1\00115=BTC\001100001=10.0\001100002=10.0\00110000"
  "3=0.0000\001100004=0.0000\001100005=0.0\001100006=0.0\00110001"
  "1=0.0\001100013=10.0\00110=004\001";
}  // namespace

void BM_fix_user_response_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto user_response = fix::UserResponse::parse(message);
          if (!user_response.user_request_id.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_user_response_parse_message);

void BM_fix_parser_dispatch_user_response(benchmark::State& state) {
  std::vector<std::byte> buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
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
                [](const fix::OrderCancelReject& order_cancel_reject) {
                },
                [](const fix::PositionReport& position_report) {
                },
                [](const fix::Reject& reject) {
                },
                [](const fix::ResendRequest& resend_request) {
                },
                [](const fix::SecurityList& security_list) {
                },
                [](const fix::TestRequest& test_request) {
                },
                [&](const fix::UserResponse& user_response) {
                  if (!user_response.user_request_id.empty())
                    ++processed;
                },
              },
              message,
              buffer);
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_parser_dispatch_user_response);
