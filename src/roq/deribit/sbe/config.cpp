/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/sbe/config.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace sbe {

// === HELPERS ===

namespace {
template <typename R>
auto create_ports(auto &path) {
  using result_type = std::remove_cvref_t<R>;
  if (std::empty(path)) {
    return result_type{};
  }
  std::ifstream stream{std::string{path}};
  if (!stream.is_open()) {
    auto what = fmt::format(R"(Unable to open path="{}")"sv, path);
    throw std::runtime_error{what};
  }
  auto doc = nlohmann::json::parse(stream);
  auto ports = doc["ports"sv];
  auto events_port = ports["events"sv].template get<uint16_t>();
  auto snapshot_port = ports["snapshot"sv].template get<uint16_t>();
  return result_type{events_port, snapshot_port};
}

template <typename R>
auto create_connections(auto &path, auto &channel_ids) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  if (std::empty(path)) {
    return result;
  }
  std::ifstream stream{std::string{path}};
  if (!stream.is_open()) {
    auto what = fmt::format(R"(Unable to open path="{}")"sv, path);
    throw std::runtime_error{what};
  }
  auto doc = nlohmann::json::parse(stream);
  auto channels = doc["channels"sv];
  for (auto &[channel_id, item] : channels.items()) {
    auto iter = std::ranges::find(channel_ids, channel_id);
    if (iter == std::end(channel_ids)) {
      continue;
    }
    auto currency_product = item["currency_product"sv].template get<std::string>();
    auto address = item["address"sv].template get<std::string>();
    auto connection = Config::Connection{
        .currency_product = currency_product,
        .address = address,
    };
    result.emplace(channel_id, std::move(connection));
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Config::Config(std::string_view const &filename, std::vector<std::string> const &channel_ids)
    : ports_{create_ports<decltype(ports_)>(filename)}, connections_{create_connections<decltype(connections_)>(filename, channel_ids)} {
}

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
