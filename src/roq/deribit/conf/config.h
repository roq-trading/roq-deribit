/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "roq/server.h"

#include "roq/deribit/conf/account.h"
#include "roq/deribit/conf/connection.h"

namespace roq {
namespace deribit {
namespace conf {

class Config final
    : public server::Config,
      public server::ConfigReader::Handler {
 public:
  Config(
      const std::string& directory,
      const std::string& file,
      const std::string& variables);

 protected:
  // server::Config
  void dispatch(server::Config::Handler& handler) const override;

  // server::ConfigReader::Handler
  void on(const ucl::Ucl& ucl, roq::Account&& account) override;
  void on(const ucl::Ucl& ucl, roq::User&& user) override;
  void on(const ucl::Ucl& ucl, const std::string& key) override;

 public:
  // cctz::time_zone time_zone;
  std::vector<roq::User> users;
  std::unordered_map<std::string, std::unordered_set<std::string> > symbols;
  std::unordered_map<std::string, Account> accounts;
  Connection rest_api;
  Connection websocket_api;
  Connection fix_api;
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
