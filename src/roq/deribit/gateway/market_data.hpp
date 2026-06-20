/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <vector>

#include "roq/utils/container.hpp"

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/io/net/connection_factory.hpp"
#include "roq/io/net/connection_manager.hpp"

#include "roq/core/download.hpp"

#include "roq/server.hpp"

#include "roq/deribit/gateway/account.hpp"
#include "roq/deribit/gateway/shared.hpp"

// session
#include "roq/deribit/protocol/fix/heartbeat.hpp"
#include "roq/deribit/protocol/fix/logon.hpp"
#include "roq/deribit/protocol/fix/logout.hpp"
#include "roq/deribit/protocol/fix/resend_request.hpp"
#include "roq/deribit/protocol/fix/test_request.hpp"

// business (inbound)
#include "roq/deribit/protocol/fix/market_data_incremental_refresh.hpp"
#include "roq/deribit/protocol/fix/market_data_request_reject.hpp"
#include "roq/deribit/protocol/fix/market_data_snapshot_full_refresh.hpp"
#include "roq/deribit/protocol/fix/security_list.hpp"
#include "roq/deribit/protocol/fix/security_status.hpp"

// business (outbound)
#include "roq/deribit/protocol/fix/market_data_request.hpp"
#include "roq/deribit/protocol/fix/security_list_request.hpp"
#include "roq/deribit/protocol/fix/security_status_request.hpp"

namespace roq {
namespace deribit {
namespace gateway {

struct MarketData final : public io::net::ConnectionManager::Handler {
  struct SymbolsUpdate final {
    std::vector<Symbol> &symbols;
  };

  struct Handler {
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  MarketData(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &, size_t index, bool master);

  MarketData(MarketData const &) = delete;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  void subscribe(size_t start_from = 0);

 protected:
  // helpers

  void operator()(Trace<protocol::fix::Heartbeat> const &, fix::Header const &);
  void operator()(Trace<protocol::fix::Logon> const &, fix::Header const &);
  void operator()(Trace<protocol::fix::Logout> const &, fix::Header const &);
  void operator()(Trace<protocol::fix::ResendRequest> const &, fix::Header const &);
  void operator()(Trace<protocol::fix::TestRequest> const &, fix::Header const &);

  void operator()(Trace<protocol::fix::SecurityList> const &, fix::Header const &);
  void operator()(Trace<protocol::fix::SecurityStatus> const &, fix::Header const &);

  void operator()(Trace<protocol::fix::MarketDataIncrementalRefresh> const &, fix::Header const &);
  void operator()(Trace<protocol::fix::MarketDataRequestReject> const &, fix::Header const &);
  void operator()(Trace<protocol::fix::MarketDataSnapshotFullRefresh> const &, fix::Header const &);

  // io::net::ConnectionManager::Handler

  void operator()(io::net::ConnectionManager::Connected const &) override;
  void operator()(io::net::ConnectionManager::Disconnected const &) override;
  void operator()(io::net::ConnectionManager::Read const &) override;
  void operator()(io::net::ConnectionManager::Write const &) override;

  // helpers

  bool ready() const { return ready_; }

  void operator()(ConnectionStatus, std::string_view const &reason = {});

  void send_logon();
  void send_logout(std::string_view const &text);
  void send_heartbeat(std::string_view const &test_req_id);
  void send_test_request(std::chrono::nanoseconds now);

  enum class State {
    UNDEFINED = 0,
    SECURITIES,
    SUBSCRIBE,
    DONE,
  };

  uint32_t download(State);

  void download_securities();

  void subscribe(std::span<Symbol const> const &symbols);
  void unsubscribe(std::span<Symbol const> const &symbols);

  void resubscribe(std::string_view const &symbol);

  void parse(Trace<fix::Message> const &);
  void parse_helper(Trace<fix::Message> const &);

  template <typename T>
  void send(T const &event);

  template <typename T>
  void send(T const &event, std::chrono::nanoseconds sending_time);

  void check(fix::Header const &);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  size_t const index_;
  bool const master_;
  bool const publish_market_by_price_;
  bool const publish_trade_summary_;
  Mask<SupportType> const supports_;
  // flags
  std::string const exchange_;
  bool const fix_debug_;
  std::chrono::nanoseconds const fix_request_timeout_;
  std::chrono::nanoseconds const fix_ping_freq_;
  // connection
  std::unique_ptr<io::net::ConnectionFactory> const connection_factory_;
  std::unique_ptr<io::net::ConnectionManager> const connection_manager_;
  // buffers
  std::vector<std::byte> encode_buffer_;
  std::vector<std::byte> decode_buffer_;
  std::string request_id_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse, security_list, security_status, market_data_incremental_refresh, market_data_request_reject,
        market_data_snapshot_full_refresh, market_data_request;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
  // state
  struct {
    uint64_t msg_seq_num = {};
  } outbound_;
  struct {
    uint64_t msg_seq_num = {};
  } inbound_;
  // account
  Account &account_;
  // cache
  Shared &shared_;
  // state
  bool ready_ = false;
  std::chrono::nanoseconds next_heartbeat_ = {};
  ConnectionStatus connection_status_ = {};
  core::Download<State> download_;
  std::chrono::nanoseconds last_logon_or_heartbeat_ = {};
  utils::unordered_set<std::string> latch_;
  // EXPERIMENTAL
  std::chrono::nanoseconds test_disconnect_time_ = {};
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
