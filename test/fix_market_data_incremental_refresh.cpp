/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/market_data_incremental_refresh.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_market_data_incremental_refresh, parse_message_1) {
  const char* message =
    "8=FIX.4.4\0019=216\00135=X\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=126\00152=20190907-15:37:00.896\00155=BTC-27SEP19\001"
    "100087=10831047\001100090=10517.4400\001746=9465994.0000\00126"
    "2=123\001268=1\001279=0\001269=1\001270=10523.0000\001271=1000"
    ".0000\001272=20190907-15:37:00.896\00110=241\001";
  std::vector<std::byte> buffer(4096);
  int results = 0;
  auto bytes = core::fix::Reader::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH);
        auto result = fix::MarketDataIncrementalRefresh::parse(message, buffer);
        /*
        EXPECT_EQ(result.heart_bt_int, uint32_t{10});
        EXPECT_EQ(result.raw_data, "1567361081237.a4eRAaHHZFio1qzVREsquVME0v1Mon1wtnxnERu7J0Y=");
        EXPECT_EQ(result.username, "5MP40u9h");
        EXPECT_EQ(result.password, "7WqHmj/pylNgnWj3V6bD0M9ULDdBh+i+Q5eZ6Z90Jzw=");
        EXPECT_EQ(result.deribit_cancel_on_disconnect, true);
        EXPECT_EQ(result.deribit_use_wordsafe_tags, false);
        */
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}

TEST(fix_market_data_incremental_refresh, parse_message_2) {
  const char* message =
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
  std::vector<std::byte> buffer(4096);
  int results = 0;
  auto bytes = core::fix::Reader::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH);
        auto result = fix::MarketDataIncrementalRefresh::parse(message, buffer);
        /*
        EXPECT_EQ(result.heart_bt_int, uint32_t{10});
        EXPECT_EQ(result.raw_data, "1567361081237.a4eRAaHHZFio1qzVREsquVME0v1Mon1wtnxnERu7J0Y=");
        EXPECT_EQ(result.username, "5MP40u9h");
        EXPECT_EQ(result.password, "7WqHmj/pylNgnWj3V6bD0M9ULDdBh+i+Q5eZ6Z90Jzw=");
        EXPECT_EQ(result.deribit_cancel_on_disconnect, true);
        EXPECT_EQ(result.deribit_use_wordsafe_tags, false);
        */
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
