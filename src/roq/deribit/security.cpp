/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/security.h"

namespace roq {
namespace deribit {

Security::Security(const Config &config, const std::string_view &account)
    : account_(account), key_(config.get_access_key(account)),
      hasher_(config.get_access_secret(account)) {
}

std::string Security::create_nonce() {
  return hasher_.create_nonce();
}

std::string Security::create_raw_data(std::chrono::milliseconds timestamp) {
  return hasher_.create_raw_data(timestamp);
}

std::string Security::create_password(const std::string_view &raw_data) {
  return hasher_.create_password(raw_data);
}

}  // namespace deribit
}  // namespace roq
