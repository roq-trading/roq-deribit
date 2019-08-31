/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/gateway.h"

// #include <cctz/time_zone.h>

#include <iomanip>
#include <string>
#include <utility>

#include "roq/logging.h"

namespace roq {
namespace deribit {

namespace {
#if (1)
static const char *REST_URI = "https://test.deribit.com/api/v2";
static const char *WS_URI = "wss://test.deribit.com/ws/api/v2";
static const char *FIX_URI = "tcp://test.deribit.com:9881";
static const char *ACCESS_KEY = "5MP40u9h";
static const char *ACCESS_SECRET = "8XC2sDXtrGFtVdOFKCU2eg3uE1oOntCoJCM3abpNBmI";
#else
static const char *REST_URI = "https://deribit.com/api/v2";
static const char *WS_URI = "wss://deribit.com/ws/api/v2";
static const char *FIX_URI = "tcp://www.deribit.com:9880";
static const char *ACCESS_KEY = "2tZQEQRV";
static const char *ACCESS_SECRET = "saQaP6WmDefitTmd6DcAqnhJFtpC9eubZ3bzYm21af4";
#endif
}  // namespace

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const conf::Config& config)
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
          core::URI(FIX_URI),
          ACCESS_KEY,
          ACCESS_SECRET) {
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
