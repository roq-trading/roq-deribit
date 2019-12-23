/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/execution_report.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_execution_report, parse_message) {
  const char *message =
    "8=FIX.4.4\0019=275\00135=8\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=2\00152=20190908-17:18:38.983\00137=2831903667\00111="
    "2831903667\00141=123\001150=I\00139=4\00154=1\00160=20190908-1"
    "7:18:38.983\001151=1\00114=0\00138=1\00140=2\00144=0.5000\0011"
    "03=0\00158=success\001207=DERIBITSERVER\00155=BTC-27SEP19\0018"
    "54=1\001231=10.0000\0016=0.000\001210=1\001100010=roq;123;345\001"
    "10=195\001";
  std::vector<std::byte> buffer(4096);
  core::fix::Buffer decode_buffer(buffer);
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::EXECUTION_REPORT);
        auto result = fix::ExecutionReport::parse(message, decode_buffer);
        EXPECT_EQ(result.order_id, "2831903667");
        EXPECT_EQ(result.cl_ord_id, "2831903667");
        EXPECT_EQ(result.orig_cl_ord_id, "123");
        EXPECT_EQ(result.exec_type, core::fix::ExecType::ORDER_STATUS);
        EXPECT_EQ(result.ord_status, core::fix::OrdStatus::CANCELED);
        EXPECT_EQ(result.side, core::fix::Side::BUY);
        // 60
        EXPECT_DOUBLE_EQ(result.leaves_qty, 1.0);
        EXPECT_DOUBLE_EQ(result.cum_qty, 0.0);
        EXPECT_DOUBLE_EQ(result.order_qty, 1.0);
        EXPECT_EQ(result.ord_type, core::fix::OrdType::LIMIT);
        EXPECT_DOUBLE_EQ(result.price, 0.5);
        EXPECT_EQ(result.ord_rej_reason, core::fix::OrdRejReason::BROKER_EXCHANGE_OPTION);
        EXPECT_EQ(result.text, "success");
        EXPECT_EQ(result.security_exchange, "DERIBITSERVER");
        EXPECT_EQ(result.symbol, "BTC-27SEP19");
        EXPECT_EQ(result.qty_type, core::fix::QtyType::CONTRACTS);
        EXPECT_DOUBLE_EQ(result.contract_multiplier, 10.0);
        EXPECT_DOUBLE_EQ(result.avg_px, 0.0);
        EXPECT_DOUBLE_EQ(result.max_show, 1.0);
        EXPECT_EQ(result.deribit_label, "roq;123;345");
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}

TEST(fix_execution_report, parse_order_mass_status) {
  const char *message =
    "8=FIX.4.4\0019=112\00135=8\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=4\00152=20190909-07:58:54.679\001584=roq-oms-005\0015"
    "85=7\00158=total_reports\001911=1\00110=045\001";
  std::vector<std::byte> buffer(4096);
  core::fix::Buffer decode_buffer(buffer);
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::EXECUTION_REPORT);
        auto result = fix::ExecutionReport::parse(message, decode_buffer);
        EXPECT_EQ(result.mass_status_req_id, "roq-oms-005");
        EXPECT_EQ(result.mass_status_req_type, core::fix::MassStatusReqType::ORDERS);
        EXPECT_EQ(result.tot_num_reports, uint32_t{1});
        EXPECT_EQ(result.text, "total_reports");
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}

TEST(fix_execution_report, parse_fill) {
  const char *message =
    "8=FIX.4.4\0019=403\00135=8\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=598\00152=20191027-14:02:33.897\00137=3026811591\0011"
    "1=3026811591\00141=roq:000000014\001150=I\00139=2\00154=1\0016"
    "0=20191027-14:02:33.897\00112=-0.00000021\001151=0\00114=1\001"
    "38=1\00140=2\00144=9593.5000\001103=0\00158=notification\00120"
    "7=DERIBITSERVER\00155=BTC-27DEC19\001854=1\001231=10.0000\0016"
    "=9593.504\001210=1\001100010=roq:1:1:1000\00132=1.0000\00131=9"
    "593.5000\0011362=1\0011363=BTC-27DEC19#2350428\0011364=9593.50"
    "00\0011365=1.0000\0011443=1\00110=177\001";
  std::vector<std::byte> buffer(4096);
  core::fix::Buffer decode_buffer(buffer);
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::EXECUTION_REPORT);
        auto result = fix::ExecutionReport::parse(message, decode_buffer);
        EXPECT_EQ(result.order_id, "3026811591");
        EXPECT_EQ(result.cl_ord_id, "3026811591");
        EXPECT_EQ(result.orig_cl_ord_id, "roq:000000014");
        EXPECT_EQ(result.exec_type, core::fix::ExecType::ORDER_STATUS);
        EXPECT_EQ(result.ord_status, core::fix::OrdStatus::FILLED);
        EXPECT_EQ(result.side, core::fix::Side::BUY);
        // 60
        EXPECT_DOUBLE_EQ(result.commission, -0.00000021);
        EXPECT_DOUBLE_EQ(result.leaves_qty, 0.0);
        EXPECT_DOUBLE_EQ(result.cum_qty, 1.0);
        EXPECT_DOUBLE_EQ(result.order_qty, 1.0);
        EXPECT_EQ(result.ord_type, core::fix::OrdType::LIMIT);
        EXPECT_DOUBLE_EQ(result.price, 9593.5);
        EXPECT_EQ(result.ord_rej_reason, core::fix::OrdRejReason::BROKER_EXCHANGE_OPTION);
        EXPECT_EQ(result.text, "notification");
        EXPECT_EQ(result.security_exchange, "DERIBITSERVER");
        EXPECT_EQ(result.symbol, "BTC-27DEC19");
        EXPECT_EQ(result.qty_type, core::fix::QtyType::CONTRACTS);
        EXPECT_DOUBLE_EQ(result.contract_multiplier, 10.0);
        EXPECT_DOUBLE_EQ(result.avg_px, 9593.504);  // TODO(thraneh): why different? not just the commission...
        EXPECT_DOUBLE_EQ(result.max_show, 1.0);
        EXPECT_EQ(result.deribit_label, "roq:1:1:1000");
        EXPECT_DOUBLE_EQ(result.last_qty, 1.0);
        EXPECT_DOUBLE_EQ(result.last_px, 9593.5);
        EXPECT_EQ(result.fills_grp.length, size_t{1});
        // item 0
        auto& item_0 = result.fills_grp.items[0];
        EXPECT_EQ(item_0.fill_exec_id, "BTC-27DEC19#2350428");
        EXPECT_DOUBLE_EQ(item_0.fill_px, 9593.5);
        EXPECT_DOUBLE_EQ(item_0.fill_qty, 1.0);
        EXPECT_EQ(item_0.fill_liquidity_ind, core::fix::FillLiquidityInd::MAKER);
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
