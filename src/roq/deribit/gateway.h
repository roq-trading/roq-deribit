/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <thread>

#include "roq/server.h"

#include "roq/core/ssl/ssl.h"
#include "roq/core/event/event.h"

#include "roq/deribit/conf/config.h"

#include "roq/deribit/controller.h"
#include "roq/deribit/rest.h"
#include "roq/deribit/websocket.h"

namespace roq {
namespace deribit {

class Gateway final : public server::Handler {
 public:
  Gateway(
      server::Dispatcher& dispatcher,
      const conf::Config& config);

  void on(const StartEvent& event) override;
  void on(const StopEvent& event) override;
  void on(const TimerEvent& event) override;
  void on(const ConnectionStatusEvent& event) override;
  void on(const CreateOrderEvent& event) override;
  void on(const ModifyOrderEvent& event) override;
  void on(const CancelOrderEvent& event) override;

  void write(Metrics& metrics) const override;

  auto& rest() {
    return _rest;
  }
  auto& websocket() {
    return _websocket;
  }

 protected:
  void run();
  void initialize_thread();
  void on_timer();

 private:
  server::Dispatcher& _dispatcher;
  core::ssl::Context _ssl_context;
  core::event::Base _base;
  core::event::DNSBase _dns_base;
  core::event::Timer _timer;
  std::atomic<bool> _stop = {false};
  std::thread _thread;
  // ...
  core::ssl::Connection _ssl_connection;
  core::event::BufferEvent _buffer_event;
  // ...
  Rest _rest;
  // ...
  Controller _controller;
  WebSocket _websocket;
};

}  // namespace deribit
}  // namespace roq
