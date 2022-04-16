/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/multicast.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/deribit/flags/common.hpp"
#include "roq/deribit/flags/config.hpp"
#include "roq/deribit/flags/multicast.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

namespace {
const auto NAME = "mc"sv;
const Mask SUPPORTS{
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context, auto port) {
  core::net::UdpConnection connection(handler, context, port);
  return connection;
}
}  // namespace

Multicast::Multicast(
    Handler &handler, core::io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      events_(create_connection(*this, context, flags::Multicast::multicast_port_events())),
      snapshot_(create_connection(*this, context, flags::Multicast::multicast_port_snapshot())),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
      },
      shared_(shared) {
}

void Multicast::operator()(const Event<Start> &) {
}

void Multicast::operator()(const Event<Stop> &) {
}

void Multicast::operator()(const Event<Timer> &) {
}

void Multicast::operator()(const core::net::UdpConnection::Read &read) {
}

void Multicast::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE);
}

}  // namespace deribit
}  // namespace roq
