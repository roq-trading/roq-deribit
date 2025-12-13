/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/logon.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using Logon = deribit::fix::Logon;

TEST_CASE("fix_logon_parse_message", "[fix_logon]") {
  auto const message =
      "8=FIX.4.4\0019=211\00135=A\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=1\00152=20190907-16:45:58.192\001108=10\00195=58\0019"
      "6=1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=\001"
      "553=5MP40u9h\001554=j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0"
      "M=\0019001=Y\00110=115\001"sv;
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::LOGON);
    auto result = Logon::create(message_2);
    CHECK(result.heart_bt_int == uint32_t{10});
    CHECK(result.raw_data == "1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc="sv);
    CHECK(result.username == "5MP40u9h"sv);
    CHECK(result.password == "j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0M="sv);
    CHECK(result.cancel_on_disconnect == true);
    CHECK(result.use_wordsafe_tags == false);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}

TEST_CASE("fix_logon_create_message", "[fix_logon]") {
  std::vector<std::byte> buffer(4096);
  uint64_t msg_seq_num = 0;
  auto sending_time = 1568702810s;
  std::string_view raw_data = "1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc="sv;
  auto logon = Logon{
      .heart_bt_int = uint16_t{10},
      .raw_data_length = static_cast<uint32_t>(std::size(raw_data)),
      .raw_data = std::data(raw_data),
      .username = "5MP40u9h"sv,
      .password = "j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0M="sv,
      .use_wordsafe_tags = false,
      .cancel_on_disconnect = true,
      .deribit_app_id = {},
      .deribit_app_sig = {},
  };
  auto header = roq::fix::Header{
      .version = roq::fix::Version::FIX_44,
      .msg_type = decltype(logon)::MSG_TYPE,
      .sender_comp_id = "ROQ_TRADING"sv,
      .target_comp_id = "DERIBITSERVER"sv,
      .msg_seq_num = ++msg_seq_num,  // note!
      .sending_time = sending_time,
  };
  auto message = logon.encode(header, buffer);
  auto const expected =
      "8=FIX.4.4\0019=0000211\00135=A\00149=ROQ_TRADING\00156=DERIBIT"
      "SERVER\00134=1\00152=20190917-06:46:50.000\001108=10\00195=58\001"
      "96=1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=\001"
      "553=5MP40u9h\001554=j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0"
      "M=\0019001=Y\00110=032\001"sv;
  REQUIRE(std::size(message) == std::size(expected));
  for (size_t i = 0; i < std::size(message); ++i) {
    CHECK(static_cast<char>(std::data(message)[i]) == expected[i]);
  }
}
