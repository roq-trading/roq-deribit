/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <fmt/format.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/server.h"

namespace roq {
namespace deribit {

class Config final : public server::Config, public server::ConfigReader::Handler {
 public:
  explicit Config(const std::string_view &path);

  std::string get_account() const;

  auto get_access_key() const {
    using namespace roq::literals;
    if (accounts.size() != 1)
      throw std::runtime_error("More accounts not yet supported"_s);
    return (*accounts.begin()).second.login;
  }
  auto get_access_secret() const {
    using namespace roq::literals;
    if (accounts.size() != 1)
      throw std::runtime_error("More accounts not yet supported"_s);
    return (*accounts.begin()).second.secret;
  }

 protected:
  // server::Config
  void dispatch(server::Config::Handler &handler) const override;

  // server::ConfigReader::Handler
  void operator()(server::Symbols &&symbols) override;
  void operator()(server::Account &&account) override;
  void operator()(server::User &&user) override;
  void operator()(const std::string_view &key, cpptoml::base &base) override;

 public:
  std::vector<server::User> users;
  server::Symbols symbols;
  absl::flat_hash_map<std::string, server::Account> accounts;
};

}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::Config> : public roq::formatter {
  template <typename C>
  auto format(const roq::deribit::Config &value, C &ctx) {
    using namespace roq::literals;
    // FIXME(thraneh): proper
    return roq::format_to(
        ctx.out(),
        "{{"
        "users=[{}], "
        "accounts=..."
        "}}"_sv,
        fmt::join(value.users, ", "_sv));
  }
};
