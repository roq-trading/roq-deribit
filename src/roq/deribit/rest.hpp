/* Copyright (c) 2017-2026, Hans Erik Thrane */

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

#include "roq/core/json/buffer_stack.hpp"

#include "roq/deribit/request.hpp"
#include "roq/deribit/shared.hpp"

#include "roq/deribit/json/get_chart_data_ack.hpp"
#include "roq/deribit/json/get_currencies_ack.hpp"
#include "roq/deribit/json/get_instruments_ack.hpp"

namespace roq {
namespace deribit {

struct Rest final : public web::rest::Client::Handler {
  struct CurrenciesUpdate final {
    std::vector<std::string> &currencies;
  };

  struct SymbolsUpdate final {
    std::span<Symbol const> symbols;
  };

  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<ReferenceData> const &, bool is_last) = 0;
    virtual void operator()(Trace<TimeSeriesUpdate> const &, bool is_last) = 0;
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
  // web::rest::Client::Handler

  void operator()(Trace<web::rest::Client::Connected> const &) override;
  void operator()(Trace<web::rest::Client::Disconnected> const &) override;
  void operator()(Trace<web::rest::Client::Latency> const &) override;

  void operator()(ConnectionStatus, std::string_view const &reason = {});

  bool ready() const { return connection_status_ == ConnectionStatus::READY; }

  bool downloading() const { return downloading_currencies_ || downloading_instruments_; }

  void check_download();

  // currencies

  void get_currencies();
  void get_currencies_ack(Trace<web::rest::Response> const &);
  void operator()(Trace<json::GetCurrenciesAck> const &);

  // instruments

  void get_instruments();
  void get_instruments_ack(Trace<web::rest::Response> const &);
  void operator()(Trace<json::GetInstrumentsAck> const &);

  // chart-data

  void get_chart_data(std::string_view const &symbol);
  void get_chart_data_ack(Trace<web::rest::Response> const &, std::string_view const &symbol);
  void operator()(Trace<json::GetChartDataAck> const &, std::string_view const &symbol);

  // helpers

  void check_request_queue(std::chrono::nanoseconds now);

  void process_response(web::rest::Response const &, auto error_handler, auto success_handler);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // connection
  std::unique_ptr<web::rest::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile  //
        currencies,
        currencies_ack,                //
        instruments, instruments_ack,  //
        chart_data, chart_data_ack;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus connection_status_ = {};
  //
  Request &request_;
  bool downloading_currencies_ = {};
  bool downloading_instruments_ = {};
};

}  // namespace deribit
}  // namespace roq
