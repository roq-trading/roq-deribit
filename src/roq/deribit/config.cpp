/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/config.h"

#include <utility>

#include "roq/logging.h"

namespace roq {
namespace deribit {

Config::Config(const std::string_view& path) {
  server::ConfigReader::parse(*this, path);
}

std::string Config::get_account() const {
  if (accounts.size() != 1)
    throw std::runtime_error("Only supporting 1 account");
  return (*accounts.begin()).first;
}

void Config::dispatch(server::Config::Handler& handler) const {
  for (auto iter : accounts)
    handler(iter.second);
  for (auto& user : users)
    handler(user);
}

void Config::operator()(server::Symbols&& symbols) {
  for (auto& iter_1 : symbols.regex) {
    for (auto& iter_2 : iter_1.second) {
      (*this).symbols.emplace_back(std::regex(iter_2));
    }
  }
}

void Config::operator()(Account&& account) {
  accounts.emplace(account.name, account);
}

void Config::operator()(User&& user) {
  users.emplace_back(user);
}

void Config::operator()(
    const std::string_view& key,
    cpptoml::base&) {
  LOG(WARNING)("UNKNOWN KEY=\"{}\"", key);
}

}  // namespace deribit
}  // namespace roq
