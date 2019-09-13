/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/conf/config.h"

#include <utility>

#include "roq/logging.h"
#include "roq/stream.h"

namespace roq {
namespace deribit {
namespace conf {

Config::Config(const std::string_view& path) {
  server::ConfigReader::parse(*this, path);
}

void Config::dispatch(server::Config::Handler& handler) const {
  for (auto iter : accounts)
    handler.on(iter.second);
  for (auto& user : users)
    handler.on(user);
}

void Config::operator()(Account&& account) {
  accounts.emplace(account.name, account);
}

void Config::operator()(User&& user) {
  users.emplace_back(user);
}

void Config::operator()(
    const std::string& key,
    const cpptoml::base& base) {
}

std::ostream& operator<<(
    std::ostream& stream,
    const Config& value) {
  return stream << "{"
    "users=" << join(value.users) << ", "
    "symbols=" << join(value.symbols) << ", "
    "accounts=" << join(value.accounts) <<
    "}";
}

}  // namespace conf
}  // namespace deribit
}  // namespace roq
