/* Copyright (c) 2017-2019, Hans Erik Thrane */

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
  int results = 0;
  auto bytes = core::fix::Reader::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::EXECUTION_REPORT);
        auto result = fix::ExecutionReport::parse(message);
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
