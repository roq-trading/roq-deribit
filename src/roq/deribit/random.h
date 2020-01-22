/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>

#include "roq/core/crypto/sha.h"

namespace roq {
namespace deribit {

class Random final {
 public:
  explicit Random(const std::string_view& secret);

  Random(const Random&) = delete;
  Random(Random&&) = delete;

  void operator=(const Random&) = delete;
  void operator=(Random&&) = delete;

  std::string create_raw_data(const std::chrono::nanoseconds now);
  std::string create_password(const std::string_view& raw_data);

 private:
  const std::string _secret;
  core::crypto::SHA256 _sha;
};

}  // namespace deribit
}  // namespace roq
