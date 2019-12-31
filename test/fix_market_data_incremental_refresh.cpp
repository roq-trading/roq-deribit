/* Copyright (c) 2017-2020, Hans Erik Thrane */

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
  core::utils::Buffer buffer(4096);
  core::fix::Buffer decode_buffer(buffer);
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH);
        auto result = fix::MarketDataIncrementalRefresh::parse(message, decode_buffer);
        EXPECT_EQ(result.symbol, "BTC-27SEP19");
        EXPECT_DOUBLE_EQ(result.deribit_trade_volume_24h, 10831047.0);
        EXPECT_DOUBLE_EQ(result.deribit_mark_price, 10517.44);
        EXPECT_DOUBLE_EQ(result.open_interest, 9465994.0);
        EXPECT_EQ(result.md_req_id, "123");
        EXPECT_EQ(result.md_inc_grp.length, size_t{1});
        // item 0
        auto& item_0 = result.md_inc_grp.items[0];
        EXPECT_EQ(item_0.md_update_action, core::fix::MDUpdateAction::NEW);
        EXPECT_EQ(item_0.md_entry_type, core::fix::MDEntryType::OFFER);
        EXPECT_DOUBLE_EQ(item_0.md_entry_px, 10523.0);
        EXPECT_DOUBLE_EQ(item_0.md_entry_size, 1000.0);
        EXPECT_EQ(item_0.md_entry_date, std::chrono::milliseconds{1567870620896});
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
  core::utils::Buffer buffer(4096);
  core::fix::Buffer decode_buffer(buffer);
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH);
        auto result = fix::MarketDataIncrementalRefresh::parse(message, decode_buffer);
        EXPECT_EQ(result.symbol, "BTC-27SEP19");
        EXPECT_EQ(result.md_inc_grp.length, size_t{5});
        // item 0
        auto& item_0 = result.md_inc_grp.items[0];
        EXPECT_EQ(item_0.md_update_action, core::fix::MDUpdateAction::NEW);
        EXPECT_EQ(item_0.md_entry_type, core::fix::MDEntryType::TRADE);
        EXPECT_DOUBLE_EQ(item_0.md_entry_px, 10519.5);
        EXPECT_DOUBLE_EQ(item_0.md_entry_size, 826.0);
        EXPECT_EQ(item_0.md_entry_date, std::chrono::milliseconds{1567870620378});
        EXPECT_EQ(item_0.deribit_trade_id, "18254681");
        EXPECT_EQ(item_0.side, core::fix::Side::BUY);
        EXPECT_EQ(item_0.order_id, "0");
        EXPECT_EQ(item_0.secondary_order_id, "0");
        EXPECT_EQ(item_0.ord_status, core::fix::OrdStatus::FILLED);
        EXPECT_DOUBLE_EQ(item_0.index_price, 10445.93);
        EXPECT_EQ(item_0.text, "2889354");
        // item 1
        auto& item_1 = result.md_inc_grp.items[1];
        EXPECT_EQ(item_1.md_update_action, core::fix::MDUpdateAction::NEW);
        EXPECT_EQ(item_1.md_entry_type, core::fix::MDEntryType::TRADE);
        EXPECT_DOUBLE_EQ(item_1.md_entry_px, 10520.0);
        EXPECT_DOUBLE_EQ(item_1.md_entry_size, 42.0);
        EXPECT_EQ(item_1.md_entry_date, std::chrono::milliseconds{1567870620378});
        EXPECT_EQ(item_1.deribit_trade_id, "18254682");
        EXPECT_EQ(item_1.side, core::fix::Side::BUY);
        EXPECT_EQ(item_1.order_id, "0");
        EXPECT_EQ(item_1.secondary_order_id, "0");
        EXPECT_EQ(item_1.ord_status, core::fix::OrdStatus::FILLED);
        EXPECT_DOUBLE_EQ(item_1.index_price, 10445.93);
        EXPECT_EQ(item_1.text, "2889355");
        // item 2
        auto& item_2 = result.md_inc_grp.items[2];
        EXPECT_EQ(item_2.md_update_action, core::fix::MDUpdateAction::NEW);
        EXPECT_EQ(item_2.md_entry_type, core::fix::MDEntryType::TRADE);
        EXPECT_DOUBLE_EQ(item_2.md_entry_px, 10520.0);
        EXPECT_DOUBLE_EQ(item_2.md_entry_size, 42.0);
        EXPECT_EQ(item_2.md_entry_date, std::chrono::milliseconds{1567870620378});
        EXPECT_EQ(item_2.deribit_trade_id, "18254683");
        EXPECT_EQ(item_2.side, core::fix::Side::BUY);
        EXPECT_EQ(item_2.order_id, "0");
        EXPECT_EQ(item_2.secondary_order_id, "0");
        EXPECT_EQ(item_2.ord_status, core::fix::OrdStatus::FILLED);
        EXPECT_DOUBLE_EQ(item_2.index_price, 10445.93);
        EXPECT_EQ(item_2.text, "2889356");
        // item 3
        auto& item_3 = result.md_inc_grp.items[3];
        EXPECT_EQ(item_3.md_update_action, core::fix::MDUpdateAction::NEW);
        EXPECT_EQ(item_3.md_entry_type, core::fix::MDEntryType::TRADE);
        EXPECT_DOUBLE_EQ(item_3.md_entry_px, 10520.0);
        EXPECT_DOUBLE_EQ(item_3.md_entry_size, 42.0);
        EXPECT_EQ(item_3.md_entry_date, std::chrono::milliseconds{1567870620378});
        EXPECT_EQ(item_3.deribit_trade_id, "18254684");
        EXPECT_EQ(item_3.side, core::fix::Side::BUY);
        EXPECT_EQ(item_3.order_id, "0");
        EXPECT_EQ(item_3.secondary_order_id, "0");
        EXPECT_EQ(item_3.ord_status, core::fix::OrdStatus::FILLED);
        EXPECT_DOUBLE_EQ(item_3.index_price, 10445.93);
        EXPECT_EQ(item_3.text, "2889357");
        // item 4
        auto& item_4 = result.md_inc_grp.items[4];
        EXPECT_EQ(item_4.md_update_action, core::fix::MDUpdateAction::NEW);
        EXPECT_EQ(item_4.md_entry_type, core::fix::MDEntryType::TRADE);
        EXPECT_DOUBLE_EQ(item_4.md_entry_px, 10520.0);
        EXPECT_DOUBLE_EQ(item_4.md_entry_size, 27.0);
        EXPECT_EQ(item_4.md_entry_date, std::chrono::milliseconds{1567870620378});
        EXPECT_EQ(item_4.deribit_trade_id, "18254685");
        EXPECT_EQ(item_4.side, core::fix::Side::BUY);
        EXPECT_EQ(item_4.order_id, "0");
        EXPECT_EQ(item_4.secondary_order_id, "0");
        EXPECT_EQ(item_4.ord_status, core::fix::OrdStatus::FILLED);
        EXPECT_DOUBLE_EQ(item_4.index_price, 10445.93);
        EXPECT_EQ(item_4.text, "2889358");
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}

TEST(fix_market_data_incremental_refresh, parse_message_3) {
  const char* message =
    "8=FIX.4.4\0019=219\00135=X\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=16453\00152=20190928-15:48:12.831\00155=ETH-PERPETUAL"
    "\001268=1\001279=0\001269=2\001270=170.1500\001271=22.0000\001"
    "272=20190928-15:48:12.830\001100009=ETH-1192275\00154=1\00137="
    "0\001198=0\00139=1\00144=170.3600\00158=586940\00110=030\001";
  core::utils::Buffer buffer(4096);
  core::fix::Buffer decode_buffer(buffer);
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH);
        auto result = fix::MarketDataIncrementalRefresh::parse(message, decode_buffer);
        EXPECT_EQ(result.symbol, "ETH-PERPETUAL");
        EXPECT_EQ(result.md_inc_grp.length, size_t{1});
        // item 0
        auto& item_0 = result.md_inc_grp.items[0];
        EXPECT_EQ(item_0.md_update_action, core::fix::MDUpdateAction::NEW);
        EXPECT_EQ(item_0.md_entry_type, core::fix::MDEntryType::TRADE);
        EXPECT_DOUBLE_EQ(item_0.md_entry_px, 170.15);
        EXPECT_DOUBLE_EQ(item_0.md_entry_size, 22.0);
        EXPECT_EQ(item_0.md_entry_date, std::chrono::milliseconds{1569685692830});
        EXPECT_EQ(item_0.deribit_trade_id, "ETH-1192275");
        EXPECT_EQ(item_0.side, core::fix::Side::BUY);
        EXPECT_EQ(item_0.order_id, "0");
        EXPECT_EQ(item_0.secondary_order_id, "0");
        EXPECT_EQ(item_0.ord_status, core::fix::OrdStatus::PARTIALLY_FILLED);
        EXPECT_DOUBLE_EQ(item_0.index_price, 170.36);
        EXPECT_EQ(item_0.text, "586940");
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
