/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "roq/server.h"

namespace roq {
namespace deribit {
namespace conf {

class Config final
    : public server::Config,
      public server::ConfigReader::Handler {
 public:
  explicit Config(const std::string_view& path);

  auto get_access_key() const {
    if (accounts.size() != 1)
      throw std::runtime_error("More accounts not yet supported");
    return (*accounts.begin()).second.login;
  }
  auto get_access_secret() const {
    if (accounts.size() != 1)
      throw std::runtime_error("More accounts not yet supported");
    return (*accounts.begin()).second.secret;
  }

 protected:
  // server::Config
  void dispatch(server::Config::Handler& handler) const override;

  // server::ConfigReader::Handler
  void operator()(Account&& account) override;
  void operator()(User&& user) override;
  void operator()(const std::string& key, const cpptoml::base& base) override;

 public:
  std::vector<User> users;
  std::unordered_map<std::string, std::unordered_set<std::string> > symbols;
  std::unordered_map<std::string, Account> accounts;
};
/*
 * REST API
 * https://api-public.sandbox.pro.deribit.com
 *
 * Websocket Feed
 * wss://ws-feed-public.sandbox.pro.deribit.com
 *
 * FIX API
 * tcp+ssl://fix-public.sandbox.pro.deribit.com:4198
 */

std::ostream& operator<<(std::ostream&, const Config&);

}  // namespace conf
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::conf::Config> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::conf::Config& value, C& ctx) {
    // FIXME(thraneh): proper
    return format_to(
        ctx.begin(),
        "{{"
        "}}");
  }
};
