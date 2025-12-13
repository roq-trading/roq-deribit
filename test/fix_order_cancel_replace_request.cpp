/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/utils/debug/fix/message.hpp"

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
      .cl_ord_id = "123"sv,
      .orig_cl_ord_id = "123"sv,
      .deribit_label = "123"sv,
      .symbol = "BTC-27SEP19"sv,
      .currency = {},
      .side = roq::fix::Side::BUY,
      .order_qty = {1.0, Precision::_1},
      .ord_type = roq::fix::OrdType::LIMIT,
      .price = {123.45, Precision::_2},
      .exec_inst = {},
      .deribit_mm_protection = {},
  };
  auto header = roq::fix::Header{
      .version = roq::fix::Version::FIX_44,
      .msg_type = decltype(order_cancel_replace_request)::MSG_TYPE,
      .sender_comp_id = "ROQ_TRADING"sv,
      .target_comp_id = "DERIBITSERVER"sv,
      .msg_seq_num = ++msg_seq_num,  // note!
      .sending_time = sending_time,
  };
  auto message = order_cancel_replace_request.encode(header, buffer);
  auto tmp = fmt::format("{}"sv, utils::debug::fix::Message{message});
  auto expected =
      "8=FIX.4.4|9=0000134|35=G|49=ROQ_TRADING|56=DERIBITSERVER|34=1|52=20190917-06:46:50.000|11=123|41=123|100010=123|55=BTC-27SEP19|54=1|38=1.0|40=2|44=123.45|10=150|"sv;
  REQUIRE(std::size(message) == std::size(expected));
  CHECK(tmp == expected);
}
