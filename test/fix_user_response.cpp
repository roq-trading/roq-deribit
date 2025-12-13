/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/user_response.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using UserResponse = deribit::fix::UserResponse;

TEST_CASE("fix_user_response_parse_message", "[fix_user_response]") {
  auto const message =
      "8=FIX.4.4\0019=199\00135=BF\00149=DERIBITSERVER\00156=ROQ_TRAD"
      "ING\00134=3\00152=20190908-08:47:31.511\001923=123\001553=5MP4"
      "0u9h\001926=1\00115=BTC\001100001=10.0\001100002=10.0\00110000"
      "3=0.0000\001100004=0.0000\001100005=0.0\001100006=0.0\00110001"
      "1=0.0\001100013=10.0\00110=004\001"sv;
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::USER_RESPONSE);
    auto user_response = UserResponse::create(message_2);
    CHECK(user_response.user_request_id == "123"sv);
    CHECK(user_response.username == "5MP40u9h"sv);
    CHECK(user_response.user_status == roq::fix::UserStatus::LOGGED_IN);
    CHECK(user_response.currency == "BTC"sv);
    CHECK(user_response.deribit_user_equity == 10.0_a);
    CHECK(user_response.deribit_user_balance == 10.0_a);
    CHECK(user_response.deribit_user_initial_margin == 0.0_a);
    CHECK(user_response.deribit_user_maintenance_margin == 0.0_a);
    CHECK(user_response.deribit_user_unrealized_pl == 0.0_a);
    CHECK(user_response.deribit_user_realized_pl == 0.0_a);
    CHECK(user_response.deribit_user_total_pl == 0.0_a);
    CHECK(user_response.deribit_user_margin_balance == 10.0_a);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
