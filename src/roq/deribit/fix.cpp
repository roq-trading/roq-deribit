/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix.h"

#include <gflags/gflags.h>

#include <netinet/tcp.h>

#include "roq/core/debug.h"
#include "roq/core/fix/exception.h"

#include "roq/deribit/gateway.h"

#include "roq/deribit/fix/parser.h"

DEFINE_bool(log_fix,
    false,
    "log fix messages?");

namespace roq {
namespace deribit {

FIX::FIX(
    Gateway& gateway,
    core::ssl::Context& ssl_context,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    const core::URI& uri,
    size_t decode_buffer_size)
    : _gateway(gateway),
      _ssl_connection(ssl_context),
      _dns_base(dns_base),
      _uri(uri),
      _buffer_event(base),
      _decode_buffer(decode_buffer_size) {
  LOG_IF(FATAL, _uri.scheme.compare("tcp") != 0)(
      "Expected URI scheme to be \"tcp\" (got \"{}\")",
      _uri.scheme);
  _buffer_event.setcb(
      [this]() { on_read(); },
      [this](int events) { on_error(events); });
  _buffer_event.enable(EV_READ);
}

void FIX::start() {
  LOG(INFO)(
      "Connecting to host=\"{}\", port={}",
      _uri.host, _uri.get_port_with_default());
  _buffer_event.connect(
      _dns_base,
      AF_INET,
      _uri.host,
      _uri.get_port_with_default());
}

void FIX::stop() {
  _buffer_event.flush(EV_WRITE, BEV_FINISHED);
  _buffer_event.shutdown(SHUT_RDWR);
}

void FIX::send(const core::utils::Message& message) {
  VLOG(4)("send(length={})", message.length());
  // core::print_memory(message.data(), message.length());
  _buffer_event.write(message.data(), message.length());
  _buffer_event.flush(EV_WRITE, BEV_FLUSH);
}

// bufferevent:

void FIX::on_read() {
  try {
    _buffer_event.read(_buffer);
    process_data();
  } catch (std::exception& e) {
    LOG(ERROR)("Exception: what=\"{}\"", e.what());
    stop();
  }
}

void FIX::on_error(int events) {
  if (events & BEV_EVENT_CONNECTED) {
    LOG(INFO)("Connected");
    _buffer_event.setsockopt(IPPROTO_TCP, TCP_NODELAY, int{1});
    _gateway.on_fix_connected();
  } else {
    LOG(WARNING)("Disconnected");
    _gateway.on_fix_disconnected();
  }
}

// fix:

void FIX::process_data() {
  for (;;) {
    auto length = _buffer.length();
    if (length == 0)
      return;
    auto buffer = _buffer.pullup(length);
    auto bytes = core::fix::Reader<fix::FIX_VERSION>::dispatch(
        [&](const core::fix::message_t& message) {
          try {
            core::fix::Buffer decode_buffer(_decode_buffer);
            fix::Parser::dispatch(
                [&](const auto& event) {
                  _gateway(message.header, event);
                },
                message,
                decode_buffer);
          } catch (std::exception&) {
            core::print_memory(buffer, length);
            core::print_string_with_escapes(buffer, length);
            throw;
          }
        },
        buffer,
        length);
    if (bytes == 0)
      return;
    if (FLAGS_log_fix)
      core::print_string_with_escapes(buffer, bytes);
    _buffer.drain(bytes);
  }
}

}  // namespace deribit
}  // namespace roq
