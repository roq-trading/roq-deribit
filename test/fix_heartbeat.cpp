/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/heartbeat.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using Heartbeat = deribit::fix::Heartbeat;

TEST_CASE("fix_heartbeat_parse_message", "[fix_heartbeat]") {
  auto const message =
      "8=FIX.4.4\0019=89\00135=0\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=2\00152=20190908-08:47:31.503\001112=anybody in there?"
      "\00110=084\001"sv;
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::HEARTBEAT);
    auto heartbeat = Heartbeat::create(message_2);
    CHECK(heartbeat.test_req_id == "anybody in there?");
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
