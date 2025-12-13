/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/market_data_incremental_refresh.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using MarketDataIncrementalRefresh = deribit::fix::MarketDataIncrementalRefresh;

TEST_CASE("fix_market_data_incremental_refresh_parse_message_1", "fix_market_data_incremental_refresh") {
  auto const message =
      "8=FIX.4.4\0019=216\00135=X\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=126\00152=20190907-15:37:00.896\00155=BTC-27SEP19\001"
      "100087=10831047\001100090=10517.4400\001746=9465994.0000\00126"
      "2=123\001268=1\001279=0\001269=1\001270=10523.0000\001271=1000"
      ".0000\001272=20190907-15:37:00.896\00110=241\001"sv;
  std::vector<std::byte> buffer(4096);
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH);
    auto result = MarketDataIncrementalRefresh::create(message_2, buffer);
    CHECK(result.symbol == "BTC-27SEP19"sv);
    CHECK(result.trade_volume24h == 10831047.0_a);
    CHECK(result.mark_price == 10517.44_a);
    CHECK(result.open_interest == 9465994.0_a);
    CHECK(result.md_req_id == "123"sv);
    CHECK(std::size(result.no_md_entries) == size_t{1});
    // item 0
    auto &item_0 = result.no_md_entries[0];
    CHECK(item_0.md_update_action == roq::fix::MDUpdateAction::NEW);
    CHECK(item_0.md_entry_type == roq::fix::MDEntryType::OFFER);
    CHECK(item_0.md_entry_px == 10523.0_a);
    CHECK(item_0.md_entry_size == 1000.0_a);
    CHECK(item_0.md_entry_date == 1567870620896ms);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}

TEST_CASE("fix_market_data_incremental_refresh_parse_message_2", "fix_market_data_incremental_refresh") {
  auto const message =
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
      "58=2889358\00110=087\001"sv;
  std::vector<std::byte> buffer(4096);
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH);
    auto result = MarketDataIncrementalRefresh::create(message_2, buffer);
    CHECK(result.symbol == "BTC-27SEP19"sv);
    CHECK(std::size(result.no_md_entries) == size_t{5});
    // item 0
    auto &item_0 = result.no_md_entries[0];
    CHECK(item_0.md_update_action == roq::fix::MDUpdateAction::NEW);
    CHECK(item_0.md_entry_type == roq::fix::MDEntryType::TRADE);
    CHECK(item_0.md_entry_px == 10519.5_a);
    CHECK(item_0.md_entry_size == 826.0_a);
    CHECK(item_0.md_entry_date == 1567870620378ms);
    CHECK(item_0.deribit_trade_id == "18254681"sv);
    CHECK(item_0.side == roq::fix::Side::BUY);
    CHECK(item_0.order_id == "0"sv);
    CHECK(item_0.secondary_order_id == "0"sv);
    CHECK(item_0.ord_status == roq::fix::OrdStatus::FILLED);
    CHECK(item_0.index_price == 10445.93_a);
    CHECK(item_0.text == "2889354"sv);
    // item 1
    auto &item_1 = result.no_md_entries[1];
    CHECK(item_1.md_update_action == roq::fix::MDUpdateAction::NEW);
    CHECK(item_1.md_entry_type == roq::fix::MDEntryType::TRADE);
    CHECK(item_1.md_entry_px == 10520.0_a);
    CHECK(item_1.md_entry_size == 42.0_a);
    CHECK(item_1.md_entry_date == 1567870620378ms);
    CHECK(item_1.deribit_trade_id == "18254682"sv);
    CHECK(item_1.side == roq::fix::Side::BUY);
    CHECK(item_1.order_id == "0"sv);
    CHECK(item_1.secondary_order_id == "0"sv);
    CHECK(item_1.ord_status == roq::fix::OrdStatus::FILLED);
    CHECK(item_1.index_price == 10445.93_a);
    CHECK(item_1.text == "2889355"sv);
    // item 2
    auto &item_2 = result.no_md_entries[2];
    CHECK(item_2.md_update_action == roq::fix::MDUpdateAction::NEW);
    CHECK(item_2.md_entry_type == roq::fix::MDEntryType::TRADE);
    CHECK(item_2.md_entry_px == 10520.0_a);
    CHECK(item_2.md_entry_size == 42.0_a);
    CHECK(item_2.md_entry_date == 1567870620378ms);
    CHECK(item_2.deribit_trade_id == "18254683"sv);
    CHECK(item_2.side == roq::fix::Side::BUY);
    CHECK(item_2.order_id == "0"sv);
    CHECK(item_2.secondary_order_id == "0"sv);
    CHECK(item_2.ord_status == roq::fix::OrdStatus::FILLED);
    CHECK(item_2.index_price == 10445.93_a);
    CHECK(item_2.text == "2889356"sv);
    // item 3
    auto &item_3 = result.no_md_entries[3];
    CHECK(item_3.md_update_action == roq::fix::MDUpdateAction::NEW);
    CHECK(item_3.md_entry_type == roq::fix::MDEntryType::TRADE);
    CHECK(item_3.md_entry_px == 10520.0_a);
    CHECK(item_3.md_entry_size == 42.0_a);
    CHECK(item_3.md_entry_date == 1567870620378ms);
    CHECK(item_3.deribit_trade_id == "18254684"sv);
    CHECK(item_3.side == roq::fix::Side::BUY);
    CHECK(item_3.order_id == "0"sv);
    CHECK(item_3.secondary_order_id == "0"sv);
    CHECK(item_3.ord_status == roq::fix::OrdStatus::FILLED);
    CHECK(item_3.index_price == 10445.93_a);
    CHECK(item_3.text == "2889357"sv);
    // item 4
    auto &item_4 = result.no_md_entries[4];
    CHECK(item_4.md_update_action == roq::fix::MDUpdateAction::NEW);
    CHECK(item_4.md_entry_type == roq::fix::MDEntryType::TRADE);
    CHECK(item_4.md_entry_px == 10520.0_a);
    CHECK(item_4.md_entry_size == 27.0_a);
    CHECK(item_4.md_entry_date == 1567870620378ms);
    CHECK(item_4.deribit_trade_id == "18254685"sv);
    CHECK(item_4.side == roq::fix::Side::BUY);
    CHECK(item_4.order_id == "0"sv);
    CHECK(item_4.secondary_order_id == "0"sv);
    CHECK(item_4.ord_status == roq::fix::OrdStatus::FILLED);
    CHECK(item_4.index_price == 10445.93_a);
    CHECK(item_4.text == "2889358"sv);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}

TEST_CASE("fix_market_data_incremental_refresh_parse_message_3", "fix_market_data_incremental_refresh") {
  auto const message =
      "8=FIX.4.4\0019=219\00135=X\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=16453\00152=20190928-15:48:12.831\00155=ETH-PERPETUAL"
      "\001268=1\001279=0\001269=2\001270=170.1500\001271=22.0000\001"
      "272=20190928-15:48:12.830\001100009=ETH-1192275\00154=1\00137="
      "0\001198=0\00139=1\00144=170.3600\00158=586940\00110=030\001"sv;
  std::vector<std::byte> buffer(4096);
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH);
    auto result = MarketDataIncrementalRefresh::create(message_2, buffer);
    CHECK(result.symbol == "ETH-PERPETUAL"sv);
    CHECK(std::size(result.no_md_entries) == size_t{1});
    // item 0
    auto &item_0 = result.no_md_entries[0];
    CHECK(item_0.md_update_action == roq::fix::MDUpdateAction::NEW);
    CHECK(item_0.md_entry_type == roq::fix::MDEntryType::TRADE);
    CHECK(item_0.md_entry_px == 170.15_a);
    CHECK(item_0.md_entry_size == 22.0_a);
    CHECK(item_0.md_entry_date == 1569685692830ms);
    CHECK(item_0.deribit_trade_id == "ETH-1192275"sv);
    CHECK(item_0.side == roq::fix::Side::BUY);
    CHECK(item_0.order_id == "0"sv);
    CHECK(item_0.secondary_order_id == "0"sv);
    CHECK(item_0.ord_status == roq::fix::OrdStatus::PARTIALLY_FILLED);
    CHECK(item_0.index_price == 170.36_a);
    CHECK(item_0.text == "586940"sv);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
