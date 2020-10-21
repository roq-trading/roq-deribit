/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/debug.h"
#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/logon.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_logon, parse_message) {
  const char *message =
      "8=FIX.4.4\0019=211\00135=A\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=1\00152=20190907-16:45:58.192\001108=10\00195=58\0019"
      "6=1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=\001"
      "553=5MP40u9h\001554=j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0"
      "M=\0019001=Y\00110=115\001";
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::LOGON);
        auto result = fix::Logon::create(message);
        EXPECT_EQ(result.heart_bt_int, uint32_t{10});
        EXPECT_EQ(
            result.raw_data,
            "1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=");
        EXPECT_EQ(result.username, "5MP40u9h");
        EXPECT_EQ(
            result.password, "j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0M=");
        EXPECT_EQ(result.cancel_on_disconnect, true);
        EXPECT_EQ(result.use_wordsafe_tags, false);
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}

TEST(fix_logon, create_message) {
  core::utils::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  std::string_view raw_data =
      "1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=";
  fix::Logon logon = {
      .heart_bt_int = uint16_t{10},
      .raw_data_length = static_cast<uint32_t>(raw_data.size()),
      .raw_data = raw_data.data(),
      .username = "5MP40u9h",
      .password = "j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0M=",
      .use_wordsafe_tags = false,
      .cancel_on_disconnect = true,
      .deribit_app_id = std::string_view(),
      .deribit_app_sig = std::string_view(),
  };
  core::fix::Writer writer(
      buffer,
      core::fix::Version::FIX_44,
      decltype(logon)::msg_type,
      "ROQ_TRADING",
      "DERIBITSERVER",
      msg_seq_num,
      sending_time);
  auto message = logon.encode(writer);
  // core::print_string_with_escapes(message.data(), message.length());
  constexpr auto expected =
      "8=FIX.4.4\0019=0000211\00135=A\00149=ROQ_TRADING\00156=DERIBIT"
      "SERVER\00134=1\00152=20190917-06:46:50.000\001108=10\00195=58\001"
      "96=1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=\001"
      "553=5MP40u9h\001554=j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0"
      "M=\0019001=Y\00110=032\001";
  ASSERT_EQ(message.length(), std::strlen(expected));
  for (size_t i = 0; i < message.length(); ++i)
    EXPECT_EQ(static_cast<char>(message.data()[i]), expected[i]);
}
