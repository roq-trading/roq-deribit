/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/order_cancel_replace_request.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using OrderCancelReplaceRequest = deribit::fix::OrderCancelReplaceRequest;

TEST_CASE("fix_order_cancel_replace_request_create_message", "[fix_order_cancel_replace_request]") {
  std::vector<std::byte> buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = 1568702810s;
  auto order_cancel_replace_request = OrderCancelReplaceRequest{
      .orig_cl_ord_id = "123"sv,
      .cl_ord_id = "123"sv,
      .transact_time = sending_time,
      .side = roq::fix::Side::BUY,
      .order_qty = {1.0, utils::to_decimals(1)},
      .ord_type = roq::fix::OrdType::LIMIT,
      .price = {123.45, utils::to_decimals(2)},
      .symbol = "BTC-27SEP19"sv,
      .exec_inst = {},
  };
  auto header = roq::fix::Header{
      .version = roq::fix::Version::FIX_44,
      .msg_type = decltype(order_cancel_replace_request)::msg_type,
      .sender_comp_id = "ROQ_TRADING"sv,
      .target_comp_id = "DERIBITSERVER"sv,
      .msg_seq_num = ++msg_seq_num,  // note!
      .sending_time = sending_time,
  };
  auto message = order_cancel_replace_request.encode(header, buffer);
  auto const expected =
      "8=FIX.4.4\0019=0000148\00135=G\00149=ROQ_TRADING\00156=DERIBIT"
      "SERVER\00134=1\00152=20190917-06:46:50.000\00141=123\00111=123"
      "\00160=20190917-06:46:50.000\00154=1\00138=1.0\00140=2\00144=1"
      "23.45\00155=BTC-27SEP19\00110=122\001"sv;
  REQUIRE(std::size(message) == std::size(expected));
  for (size_t i = 0; i < std::size(message); ++i)
    CHECK(static_cast<char>(std::data(message)[i]) == expected[i]);
}
