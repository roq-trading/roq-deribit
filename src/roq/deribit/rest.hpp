/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/utils/container.hpp"

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/deribit/request.hpp"
#include "roq/deribit/shared.hpp"

#include "roq/deribit/json/currency.hpp"
#include "roq/deribit/json/instrument.hpp"

namespace roq {
namespace deribit {

struct Rest final : public web::rest::Client::Handler {
  struct CurrenciesUpdate final {
    std::vector<std::string> &currencies;
  };

  struct SymbolsUpdate final {
    std::vector<Symbol> &symbols;
  };

  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<ReferenceData> const &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(CurrenciesUpdate &) = 0;
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  Rest(Handler &, io::Context &context, uint16_t stream_id, Shared &, Request &);

  Rest(Rest const &) = delete;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

 protected:
  void operator()(Trace<web::rest::Client::Connected> const &) override;
  void operator()(Trace<web::rest::Client::Disconnected> const &) override;
  void operator()(Trace<web::rest::Client::Latency> const &) override;

  void operator()(ConnectionStatus);

  bool ready() const { return status_ == ConnectionStatus::READY; }

  bool downloading() const { return downloading_currencies_ || downloading_instruments_; }

  void check_download();

  void get_currencies();
  void get_currencies_ack(Trace<web::rest::Response> const &);
  void operator()(Trace<json::Currency> const &);

  void get_instruments();
  void get_instruments_ack(Trace<web::rest::Response> const &);
  bool operator()(Trace<json::Instrument> const &);

  template <typename SuccessHandler, typename ErrorHandler>
  void process_response(web::rest::Response const &, SuccessHandler, ErrorHandler);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // connection
  std::unique_ptr<web::rest::Client> const connection_;
  // buffers
  std::vector<std::byte> decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile get_instruments, get_instruments_ack;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
  // cache
  Shared &shared_;
  utils::unordered_set<std::string> symbols_;
  // state
  ConnectionStatus status_ = {};
  //
  Request &request_;
  bool downloading_currencies_ = {};
  bool downloading_instruments_ = {};
};

}  // namespace deribit
}  // namespace roq
