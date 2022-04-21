/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/multicast.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "roq/debug/hex/message.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/deribit/flags/common.hpp"
#include "roq/deribit/flags/config.hpp"
#include "roq/deribit/flags/multicast.hpp"

#include "roq/deribit/sbe/utils.hpp"

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
  std::string local_interface{flags::Multicast::local_interface()};
  struct in_addr local = {};
  local.s_addr = inet_addr(local_interface.c_str());
  for (auto &multicast_address : flags::Multicast::multicast_address()) {
    struct in_addr multicast = {};
    multicast.s_addr = inet_addr(multicast_address.c_str());
    connection.add_membership(core::NetworkAddress{multicast, 0}, core::NetworkAddress{local, 0});
  }
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
  log::info<5>("received {} byte(s)"sv, std::size(read.buffer));
  if (!sbe::Parser::dispatch(*this, read.buffer)) {
    log::warn<5>("Failed to parse message"sv);
    log::warn<5>("{}"sv, debug::hex::Message{read.buffer});
  }
}

void Multicast::operator()(const core::net::UdpConnection::Error &error) {
  log::warn<1>("Error: what={}"sv, error.what);
}

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Instrument &instrument) {
  log::info<5>(
      "channel_id={}, sequence_number={}, instrument={}"sv,
      channel_id,
      sequence_number,
      instrument);
}

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Book &book) {
  log::info<5>("channel_id={}, sequence_number={}, book={}"sv, channel_id, sequence_number, book);
}

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Quote &quote) {
  log::info<5>("channel_id={}, sequence_number={}, quote={}"sv, channel_id, sequence_number, quote);
}

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Trades &trades) {
  log::info<5>(
      "channel_id={}, sequence_number={}, trades={}"sv, channel_id, sequence_number, trades);
}

void Multicast::operator()(
    uint16_t channel_id, uint32_t sequence_number, deribit_multicast::Snapshot &snapshot) {
  log::info<5>(
      "channel_id={}, sequence_number={}, snapshot={}"sv, channel_id, sequence_number, snapshot);
}

void Multicast::operator()(metrics::Writer &writer) {
  writer  //
      .write(counter_.disconnect, metrics::COUNTER)
      .write(profile_.parse, metrics::PROFILE);
}

}  // namespace deribit
}  // namespace roq
