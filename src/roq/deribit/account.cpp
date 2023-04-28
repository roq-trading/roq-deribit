/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/account.hpp"

namespace roq {
namespace deribit {

// === IMPLEMENTATION ===

Account::Account(Config const &config, std::string_view const &name)
    : name_{name}, key_{config.get_access_key(name)}, crypto_{config.get_access_secret(name)} {
}

std::string Account::create_nonce() {
  return crypto_.create_nonce();
}

std::string Account::create_password(std::string_view const &raw_data) {
  return crypto_.create_password(raw_data);
}

}  // namespace deribit
}  // namespace roq
