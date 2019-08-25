/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/conf/config.h"

#include <utility>

#include "roq/logging.h"
#include "roq/stream.h"

namespace roq {
namespace deribit {
namespace conf {

namespace {
/*
static auto create_time_zone(
    const ucl::Ucl& ucl) {
  auto name = ucl.string_value();
  cctz::time_zone result;
  if (!cctz::load_time_zone(name, &result)) {
    std::stringstream ss;
    ss << "Unable to initialize time-zone object using "
      "time_zone=\"" << name << "\"";
    throw std::runtime_error(ss.str());
  }
  return result;
}
*/
static auto create_unordered_set(
    const ucl::Ucl& ucl) {
  std::unordered_set<std::string> result;
  switch (ucl.type()) {
    case UCL_NULL: {
      break;
    }
    case UCL_ARRAY: {
      for (size_t i = 0; i < ucl.size(); ++i) {
        auto value = ucl.at(i).string_value();
        result.emplace(value);
      }
      break;
    }
    case UCL_STRING: {
      result.emplace(ucl.string_value());
      break;
    }
    default: {
      throw std::runtime_error("Expected array or string");
    }
  }
  return result;
}
static auto create_symbols(const ucl::Ucl& ucl) {
  std::unordered_map<std::string, std::unordered_set<std::string> > result;
  for (auto& iter : ucl) {
    const auto& key = iter.key();
    try {
      result.emplace(key, create_unordered_set(iter));
    } catch (...) {
      LOG(WARNING) << "Unable to parse symbols {"
        "key=\"" << key << "\""
        "}";
      throw;
    }
  }
  return result;
}
static auto create_connection(const ucl::Ucl& ucl) {
  auto result = Connection {
    .url = ucl.lookup("url").string_value(),
  };
  if (result.url.empty())
    throw std::runtime_error("Missing field: \"url\"");
  return result;
}

}  // namespace

Config::Config(
    const std::string& directory,
    const std::string& file,
    const std::string& variables) {
  server::ConfigReader::parse(
      *this,
      directory,
      file,
      variables,
      {
        "time_zone",
        "symbols",
        "websocket_api",
        "rest_api",
        "fix_api",
      });
}

void Config::dispatch(server::Config::Handler& handler) const {
  for (auto iter : accounts)
    handler.on(iter.second.account);
  for (auto& user : users)
    handler.on(user);
}

void Config::on(const ucl::Ucl& ucl, roq::Account&& account) {
  auto key = account.name;  // avoid confusion with std::move
  accounts.emplace(
      key,
      Account {
        .account = std::move(account),
        .broker = ucl.lookup("broker").string_value(),
        .seat_no = static_cast<int>(ucl.lookup("seat_no").int_value()),
        .master = ucl.lookup("master").bool_value()
      });
}

void Config::on(const ucl::Ucl& ucl, roq::User&& user) {
  users.emplace_back(std::move(user));
}

void Config::on(const ucl::Ucl& ucl, const std::string& key) {
  if (key.compare("time_zone") == 0) {
    // time_zone = create_time_zone(ucl);
  } else if (key.compare("symbols") == 0) {
    symbols = create_symbols(ucl);
  } else if (key.compare("rest_api") == 0) {
    rest_api = create_connection(ucl);
  } else if (key.compare("websocket_api") == 0) {
    websocket_api = create_connection(ucl);
  } else if (key.compare("fix_api") == 0) {
    fix_api = create_connection(ucl);
  } else {
    LOG(FATAL) << "Unexpected";
  }
}

std::ostream& operator<<(
    std::ostream& stream,
    const Config& value) {
  return stream << "{"
    // "time_zone=" << value.time_zone.name() << ", "
    "users=" << join(value.users) << ", "
    "symbols=" << join(value.symbols) << ", "
    "accounts=" << join(value.accounts) << ", "
    "rest_api=" << value.rest_api << ", "
    "websocket_api=" << value.websocket_api << ", "
    "fix_api=" << value.fix_api <<
    "}";
}

}  // namespace conf
}  // namespace deribit
}  // namespace roq
