/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

#include <iomanip>
#include <string>
#include <utility>

#include "roq/logging.h"

namespace roq {
namespace deribit {

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const conf::Config& config,
    const core::URI& ws_uri,
    const core::URI& fix_uri)
    : _dispatcher(dispatcher),
      _dns_base(_base, true),
      _timer(_base, EV_PERSIST, [this]() { on_timer(); }),
      _ssl_connection(_ssl_context),
      _buffer_event(_base, _ssl_connection),
      _controller(*this),
      _fix(
          _controller,
          _ssl_context,
          _base,
          _dns_base,
          fix_uri,
          config.get_access_key(),
          config.get_access_secret()) {
}

void Gateway::on(const StartEvent& event) {
  LOG(INFO) << "Starting the gateway event loop...";
  _thread = std::thread([this]() { run(); });
}

void Gateway::on(const StopEvent& event) {
  LOG(INFO) << "Stopping the gateway event loop...";
  _stop.store(true, std::memory_order_release);
  if (_thread.joinable())
    _thread.join();
  LOG(INFO) << "The gateway event loop has stopped";
}

void Gateway::on(const TimerEvent& event) {
}

void Gateway::on(const ConnectionStatusEvent& event) {
}

void Gateway::on(const CreateOrderEvent& event) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::on(const ModifyOrderEvent& event) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::on(const CancelOrderEvent& event) {
  // TODO(thraneh): send ack saying we can't do this yet
}

void Gateway::write(Metrics& metrics) const {
}

void Gateway::run() {
  LOG(INFO) << "Gateway event loop has started";
  try {
    initialize_thread();
    _timer.add(std::chrono::milliseconds{100});

    _fix.start();  // FIXME(thraneh): move to Controller

    _base.loop(EVLOOP_NO_EXIT_ON_EMPTY);
  } catch (std::exception& e) {
    LOG(FATAL) << "Unhandled exception, what=\"" << e.what() << "\"";
  } catch (...) {
    LOG(FATAL) << "Unhandled exception";
  }
  LOG(INFO) << "Gateway event loop has finished";
}

void Gateway::initialize_thread() {
  // TODO(thraneh): affinity
}

void Gateway::on_timer() {
  if (_stop.load(std::memory_order_acquire))
    _base.loopbreak();
}

}  // namespace deribit
}  // namespace roq
