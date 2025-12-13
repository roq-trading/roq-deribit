/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/utils/hash/sha256.hpp"

#include "roq/utils/mac/hmac.hpp"

namespace roq {
namespace deribit {
namespace tools {

struct Crypto final {
  explicit Crypto(std::string_view const &secret);

  Crypto(Crypto &&) = delete;
  Crypto(Crypto const &) = delete;

  std::string create_nonce();

  std::pair<std::string, std::chrono::milliseconds> create_signature(std::chrono::milliseconds timestamp, std::string_view const &nonce);

  std::string create_raw_data(std::chrono::milliseconds timestamp);
  std::string create_raw_data(std::chrono::milliseconds timestamp, std::string_view const &nonce);

  std::string create_password(std::string_view const &raw_data);

 protected:
  int64_t get_sequence(std::chrono::milliseconds timestamp);

 private:
  using Hash = utils::hash::SHA256;
  using MAC = utils::mac::HMAC<utils::hash::SHA256>;
  using Digest = std::array<std::byte, MAC::DIGEST_LENGTH>;

  std::string const secret_;
  Hash hash_;
  MAC mac_;
  Digest digest_;
  std::chrono::milliseconds timestamp_ = {};
};

}  // namespace tools
}  // namespace deribit
}  // namespace roq
