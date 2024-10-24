/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/deribit/dump/controller.hpp"

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <fmt/chrono.h>

#include <deribit_multicast/MessageHeader.h>

#include "roq/logging.hpp"

#include "roq/utils/debug/hex/message.hpp"

#include "roq/deribit/sbe/parser.hpp"
#include "roq/deribit/sbe/utils.hpp"

#include "roq/deribit/dump/pcap.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace dump {

// === HELPERS ===

namespace {
auto convert(timeval ts) {
  return std::chrono::nanoseconds{std::chrono::seconds{ts.tv_sec} + std::chrono::microseconds{ts.tv_usec}};
}

struct Bridge final : public sbe::Parser::Handler {
  Bridge(struct pcap_pkthdr const *header, u_char const *packet) : header_{header}, packet_{packet} {}

 protected:
  bool operator()(sbe::Frame const &) override { return true; }

  void operator()(Trace<deribit_multicast::Instrument> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::Book> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::Trades> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::Ticker> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::Snapshot> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::SnapshotStart> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::SnapshotEnd> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::ComboLegs> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::PriceIndex> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::Rfq> const &event, sbe::Frame const &frame) override { print(event, frame); }
  void operator()(Trace<deribit_multicast::InstrumentV2> const &event, sbe::Frame const &frame) override { print(event, frame); }

  void print(auto &event, auto &frame) {
    auto timestamp = convert((*header_).ts);
    size_t offset = 0;
    auto ether_header = reinterpret_cast<struct ether_header const *>(packet_ + offset);
    auto ether_type = ntohs((*ether_header).ether_type);
    if (ether_type == ETHERTYPE_VLAN) {
      offset += 4;  // XXX FIXME find somee struct or length in system header files... (VLAN tag)
      ether_header = reinterpret_cast<struct ether_header const *>(packet_ + offset);
      ether_type = ntohs((*ether_header).ether_type);
    }
    if (ether_type == ETHERTYPE_IP) {
      offset += sizeof(struct ether_header);
      auto ip_header = reinterpret_cast<struct ip const *>(packet_ + offset);
      char src[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &((*ip_header).ip_src), src, INET_ADDRSTRLEN);
      char dst[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &((*ip_header).ip_dst), dst, INET_ADDRSTRLEN);
      if ((*ip_header).ip_p == IPPROTO_UDP) {
        offset += sizeof(struct ip);
        auto udp_header = reinterpret_cast<struct udphdr const *>(packet_ + offset);
#if __APPLE__
        // auto src_port = ntohs((*udp_header).uh_sport);
        auto dst_port = ntohs((*udp_header).uh_dport);
#else
        // auto src_port = ntohs((*udp_header).source);
        auto dst_port = ntohs((*udp_header).dest);
#endif
        using value_type = std::remove_cvref<decltype(event)>::type::value_type;
        auto &value = const_cast<value_type &>(event.value);  // note! not const-safe
        fmt::print(
            "message={{timestamp={}, address={}, port={}, channel_id={}, sequence_number={}, {}={}}}\n"sv,
            timestamp,
            dst,
            dst_port,
            frame.channel_id,
            frame.sequence_number,
            get_name<value_type>(),
            value);
      }
    } else {
      log::fatal("Unexpected: ether_type=0x{:x}"sv, ether_type);
    }
  }

 private:
  struct pcap_pkthdr const *header_;
  u_char const *packet_;
};
}  // namespace

// === IMPLEMENTATION ===

Controller::Controller(Settings const &settings, std::string_view const &pcap_path) : settings_{settings}, pcap_path_{pcap_path} {
}

void Controller::dispatch() {
  auto callback = [&](struct pcap_pkthdr const *header, u_char const *packet) -> bool {
    if (settings_.print_packet) {
      utils::debug::hex::Message message{reinterpret_cast<std::byte const *>(packet), (*header).len};
      fmt::print("packet={}\n"sv, message);
    }
    auto timestamp = convert((*header).ts);
    size_t offset = 0;
    auto ether_header = reinterpret_cast<struct ether_header const *>(packet + offset);
    auto ether_type = ntohs((*ether_header).ether_type);
    // XXX FIXME there is also VLAG double-tagging... how to identify?
    if (ether_type == ETHERTYPE_VLAN) {
      offset += 4;  // XXX FIXME find somee struct or length in system header files... (VLAN tag)
      ether_header = reinterpret_cast<struct ether_header const *>(packet + offset);
      ether_type = ntohs((*ether_header).ether_type);
    }
    if (ether_type == ETHERTYPE_IP) {
      offset += sizeof(struct ether_header);
      auto ip_header = reinterpret_cast<struct ip const *>(packet + offset);
      if ((*ip_header).ip_p == IPPROTO_UDP) {
        offset += sizeof(struct ip) + sizeof(struct udphdr);
        std::span payload{reinterpret_cast<std::byte const *>(packet + offset), (*header).len - offset};
        if (settings_.print_payload) {
          utils::debug::hex::Message message{payload};
          fmt::print("payload={}\n"sv, message);
        }
        Bridge bridge{header, packet};
        TraceInfo trace_info;
        sbe::Parser::dispatch(bridge, payload, trace_info);
      }
    } else {
      log::fatal("Unexpected: ether_type=0x{:x}"sv, ether_type);
    }
    return false;
  };
  // market_data_.start();
  PCAP{pcap_path_}.dispatch(callback);
}

}  // namespace dump
}  // namespace deribit
}  // namespace roq
