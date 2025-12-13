/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/execution_report.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using ExecutionReport = deribit::fix::ExecutionReport;

TEST_CASE("fix_execution_report_parse_message", "[fix_execution_report]") {
  auto const message =
      "8=FIX.4.4\0019=275\00135=8\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=2\00152=20190908-17:18:38.983\00137=2831903667\00111="
      "2831903667\00141=123\001150=I\00139=4\00154=1\00160=20190908-1"
      "7:18:38.983\001151=1\00114=0\00138=1\00140=2\00144=0.5000\0011"
      "03=0\00158=success\001207=DERIBITSERVER\00155=BTC-27SEP19\0018"
      "54=1\001231=10.0000\0016=0.000\001210=1\001100010=roq;123;345\001"
      "10=195\001"sv;
  std::vector<std::byte> buffer(4096);
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::EXECUTION_REPORT);
    auto result = ExecutionReport::create(message_2, buffer);
    CHECK(result.order_id == "2831903667"sv);
    CHECK(result.cl_ord_id == "2831903667"sv);
    CHECK(result.orig_cl_ord_id == "123"sv);
    CHECK(result.exec_type == roq::fix::ExecType::ORDER_STATUS);
    CHECK(result.ord_status == roq::fix::OrdStatus::CANCELED);
    CHECK(result.side == roq::fix::Side::BUY);
    // 60
    CHECK(result.leaves_qty == 1.0_a);
    CHECK(result.cum_qty == 0.0_a);
    CHECK(result.order_qty == 1.0_a);
    CHECK(result.ord_type == roq::fix::OrdType::LIMIT);
    CHECK(result.price == 0.5_a);
    CHECK(result.ord_rej_reason == roq::fix::OrdRejReason::BROKER_EXCHANGE_OPTION);
    CHECK(result.text == "success"sv);
    CHECK(result.security_exchange == "DERIBITSERVER"sv);
    CHECK(result.symbol == "BTC-27SEP19"sv);
    CHECK(result.qty_type == roq::fix::QtyType::CONTRACTS);
    CHECK(result.contract_multiplier == 10.0_a);
    CHECK(result.avg_px == 0.0_a);
    CHECK(result.max_show == 1.0_a);
    CHECK(result.deribit_label == "roq;123;345"sv);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}

TEST_CASE("fix_execution_report_parse_order_mass_status", "[fix_execution_report]") {
  auto const message =
      "8=FIX.4.4\0019=112\00135=8\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=4\00152=20190909-07:58:54.679\001584=roq-oms-005\0015"
      "85=7\00158=total_reports\001911=1\00110=045\001"sv;
  std::vector<std::byte> buffer(4096);
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::EXECUTION_REPORT);
    auto result = ExecutionReport::create(message_2, buffer);
    CHECK(result.mass_status_req_id == "roq-oms-005"sv);
    CHECK(result.mass_status_req_type == roq::fix::MassStatusReqType::ORDERS);
    CHECK(result.tot_num_reports == uint32_t{1});
    CHECK(result.text == "total_reports"sv);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}

TEST_CASE("fix_execution_report_parse_fill", "[fix_execution_report]") {
  auto const message =
      "8=FIX.4.4\0019=403\00135=8\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=598\00152=20191027-14:02:33.897\00137=3026811591\0011"
      "1=3026811591\00141=roq:000000014\001150=I\00139=2\00154=1\0016"
      "0=20191027-14:02:33.897\00112=-0.00000021\001151=0\00114=1\001"
      "38=1\00140=2\00144=9593.5000\001103=0\00158=notification\00120"
      "7=DERIBITSERVER\00155=BTC-27DEC19\001854=1\001231=10.0000\0016"
      "=9593.504\001210=1\001100010=roq:1:1:1000\00132=1.0000\00131=9"
      "593.5000\0011362=1\0011363=BTC-27DEC19#2350428\0011364=9593.50"
      "00\0011365=1.0000\0011443=1\00110=177\001"sv;
  std::vector<std::byte> buffer(4096);
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::EXECUTION_REPORT);
    auto result = ExecutionReport::create(message_2, buffer);
    CHECK(result.order_id == "3026811591"sv);
    CHECK(result.cl_ord_id == "3026811591"sv);
    CHECK(result.orig_cl_ord_id == "roq:000000014"sv);
    CHECK(result.exec_type == roq::fix::ExecType::ORDER_STATUS);
    CHECK(result.ord_status == roq::fix::OrdStatus::FILLED);
    CHECK(result.side == roq::fix::Side::BUY);
    // 60
    CHECK(result.commission == -0.00000021_a);
    CHECK(result.leaves_qty == 0.0_a);
    CHECK(result.cum_qty == 1.0_a);
    CHECK(result.order_qty == 1.0_a);
    CHECK(result.ord_type == roq::fix::OrdType::LIMIT);
    CHECK(result.price == 9593.5_a);
    CHECK(result.ord_rej_reason == roq::fix::OrdRejReason::BROKER_EXCHANGE_OPTION);
    CHECK(result.text == "notification"sv);
    CHECK(result.security_exchange == "DERIBITSERVER"sv);
    CHECK(result.symbol == "BTC-27DEC19"sv);
    CHECK(result.qty_type == roq::fix::QtyType::CONTRACTS);
    CHECK(result.contract_multiplier == 10.0_a);
    CHECK(result.avg_px == 9593.504_a);  // TODO(thraneh): why different? not just
                                         // the commission...
    CHECK(result.max_show == 1.0_a);
    CHECK(result.deribit_label == "roq:1:1:1000"sv);
    CHECK(result.last_qty == 1.0_a);
    CHECK(result.last_px == 9593.5_a);
    CHECK(std::size(result.no_fills) == size_t{1});
    // item 0
    auto &item_0 = result.no_fills[0];
    CHECK(item_0.fill_exec_id == "BTC-27DEC19#2350428"sv);
    CHECK(item_0.fill_px == 9593.5_a);
    CHECK(item_0.fill_qty == 1.0_a);
    CHECK(item_0.fill_liquidity_ind == roq::fix::FillLiquidityInd::ADDED_LIQUIDITY);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
