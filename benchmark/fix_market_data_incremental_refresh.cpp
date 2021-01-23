/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/market_data_incremental_refresh.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
const char *message_1 =
    "8=FIX.4.4\0019=216\00135=X\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=126\00152=20190907-15:37:00.896\00155=BTC-27SEP19\001"
    "100087=10831047\001100090=10517.4400\001746=9465994.0000\00126"
    "2=123\001268=1\001279=0\001269=1\001270=10523.0000\001271=1000"
    ".0000\001272=20190907-15:37:00.896\00110=241\001";
const char *message_2 =
    "8=FIX.4.4\0019=726\00135=X\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=117\00152=20190907-15:37:00.384\00155=BTC-27SEP19\001"
    "268=5\001279=0\001269=2\001270=10519.5000\001271=826.0000\0012"
    "72=20190907-15:37:00.378\001100009=18254681\00154=1\00137=0\001"
    "198=0\00139=2\00144=10445.9300\00158=2889354\001279=0\001269=2"
    "\001270=10520.0000\001271=42.0000\001272=20190907-15:37:00.378"
    "\001100009=18254682\00154=1\00137=0\001198=0\00139=2\00144=104"
    "45.9300\00158=2889355\001279=0\001269=2\001270=10520.0000\0012"
    "71=42.0000\001272=20190907-15:37:00.378\001100009=18254683\001"
    "54=1\00137=0\001198=0\00139=2\00144=10445.9300\00158=2889356\001"
    "279=0\001269=2\001270=10520.0000\001271=42.0000\001272=2019090"
    "7-15:37:00.378\001100009=18254684\00154=1\00137=0\001198=0\001"
    "39=2\00144=10445.9300\00158=2889357\001279=0\001269=2\001270=1"
    "0520.0000\001271=27.0000\001272=20190907-15:37:00.378\00110000"
    "9=18254685\00154=1\00137=0\001198=0\00139=2\00144=10445.9300\001"
    "58=2889358\00110=087\001";
}  // namespace

void BM_fix_market_data_increment_refresh_parse_message_1(
    benchmark::State &state) {  // cppcheck-suppress constParameterCallback
  core::utils::Buffer buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Buffer decode_buffer(buffer);
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t &message) {
          auto result =
              fix::MarketDataIncrementalRefresh::create(message, decode_buffer);
          if (!result.md_req_id.empty())
            ++processed;
        },
        message_1,
        std::strlen(message_1));
  }
}

BENCHMARK(BM_fix_market_data_increment_refresh_parse_message_1);

void BM_fix_market_data_increment_refresh_parse_message_2(
    benchmark::State &state) {  // cppcheck-suppress constParameterCallback
  core::utils::Buffer buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Buffer decode_buffer(buffer);
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t &message) {
          auto result =
              fix::MarketDataIncrementalRefresh::create(message, decode_buffer);
          if (!result.md_req_id.empty())
            ++processed;
        },
        message_2,
        std::strlen(message_2));
  }
}

BENCHMARK(BM_fix_market_data_increment_refresh_parse_message_2);
