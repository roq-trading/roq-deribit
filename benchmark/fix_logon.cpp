/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=211\00135=A\00149=DERIBITSERVER\00156=ROQ_TRADI"
  "NG\00134=1\00152=20190907-16:45:58.192\001108=10\00195=58\0019"
  "6=1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=\001"
  "553=5MP40u9h\001554=j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0"
  "M=\0019001=Y\00110=115\001";
}  // namespace

void BM_fix_logon_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto result = fix::Logon::parse(message);
          if (result.heart_bt_int > 0)
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_logon_parse_message);

void BM_fix_parser_dispatch_logon(benchmark::State& state) {
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
                [&](const fix::Logon& logon) {
                  if (logon.heart_bt_int > 0)
                    ++processed;
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

BENCHMARK(BM_fix_parser_dispatch_logon);

void BM_fix_logon_create_message(benchmark::State& state) {
  core::utils::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  uint64_t processed = 0;
  for (auto _ : state) {
    fix::Logon logon = {
      .heart_bt_int = uint16_t{10},
      .raw_data = "1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=",
      .username = "5MP40u9h",
      .password = "j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0M=",
      .deribit_cancel_on_disconnect = true,
    };
    auto message = logon.encode(
        buffer,
        msg_seq_num,
        sending_time);
    if (message.length())
      ++processed;
  }
}

BENCHMARK(BM_fix_logon_create_message);
