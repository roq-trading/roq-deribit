/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/logout.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using Logout = deribit::fix::Logout;

TEST_CASE("fix_logout_parse_message", "[fix_logout]") {
  auto const message =
      "8=FIX.4.4\0019=90\00135=5\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=1\00152=20190907-16:56:43.398\00158=invalid_credential"
      "s\00110=166\001"sv;
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::LOGOUT);
    auto logout = Logout::create(message_2);
    CHECK(logout.text == "invalid_credentials");
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
