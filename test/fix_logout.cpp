/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/fix/reader.hpp"

#include "roq/deribit/fix/logout.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("fix_logout_parse_message", "[fix_logout]") {
  const auto message =
      "8=FIX.4.4\0019=90\00135=5\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=1\00152=20190907-16:56:43.398\00158=invalid_credential"
      "s\00110=166\001"sv;
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::Message &message) {
        ++results;
        CHECK(message.header.msg_type == core::fix::MsgType::LOGOUT);
        auto logout = fix::Logout::create(message);
        CHECK(logout.text == "invalid_credentials");
      },
      message);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
