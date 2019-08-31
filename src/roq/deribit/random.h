/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>

namespace roq {
namespace deribit {

struct Random final {
  static std::string create_raw_data(
      const std::chrono::nanoseconds now);
  static std::string create_password(
      const std::string_view& raw_data,
      const std::string_view& access_secret);
};

}  // namespace deribit
}  // namespace roq
