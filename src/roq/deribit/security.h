/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>

#include "roq/core/crypto/hmac.h"
#include "roq/core/crypto/sha.h"

#include "roq/deribit/config.h"

namespace roq {
namespace deribit {

class Security final {
 public:
  explicit Security(const Config &);

  Security(Security &&) = delete;
  Security(const Security &) = delete;

  std::string_view get_access_key() const { return key_; }

  std::string create_nonce();

  std::string create_signature(std::chrono::milliseconds timestamp, const std::string_view &nonce);

  std::string create_raw_data(const std::chrono::nanoseconds now);
  std::string create_password(const std::string_view &raw_data);

 private:
  const std::string key_;
  const std::string secret_;
  core::crypto::SHA256 sha_;
  core::crypto::HMAC_SHA256 hmac_;
};

}  // namespace deribit
}  // namespace roq
