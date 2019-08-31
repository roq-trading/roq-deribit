/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix.h"

#include <openssl/sha.h>

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <cinttypes>
#include <random>

#include "roq/patterns.h"

#include "roq/core/base64.h"
#include "roq/core/clock.h"
#include "roq/core/debug.h"

#include "roq/core/fix/reader.h"
#include "roq/core/fix/writer.h"

#include "roq/deribit/random.h"

namespace roq {
namespace deribit {

namespace {
constexpr auto PING_FREQUENCY = std::chrono::seconds{10};
constexpr auto DECODE_BUFFER_SIZE = size_t{1048576};  // FIXME(thraneh): flag
static std::random_device RANDOM_DEVICE;
static std::uniform_int_distribution<uint32_t> DISTRIBUTION;
constexpr uint32_t DERIBIT_FIX_FIELD_CANCEL_ON_DISCONNECT = 9001;
constexpr const char *SENDER_COMP_ID = "ROQ_TRADING";
constexpr const char *TARGET_COMP_ID = "DERIBITSERVER";
}  // namespace

FIX::FIX(
    Controller& controller,
    core::ssl::Context& ssl_context,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    const core::URI& uri,
    const std::string_view& access_key,
    const std::string_view& access_secret)
    : _controller(controller),
      _ssl_connection(ssl_context),
      _dns_base(dns_base),
      _uri(uri),
      _access_key(access_key),
      _access_secret(access_secret),
      _timer(base, EV_PERSIST, [this]() { on_timer(); }),
      _buffer_event(base),  //, _ssl_connection),
      _decode_buffer(DECODE_BUFFER_SIZE) {
  LOG_IF(FATAL, _uri.scheme.compare("tcp") != 0) <<
    "Expected URI scheme to be \"tcp\" (got \"" << _uri.scheme << "\")";
  int value = 1;
  setsockopt(_buffer_event.getfd(), IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
  _timer.add(std::chrono::seconds{1});
  _buffer_event.setcb(
      [this]() { on_read(); },
      [this](int events) { on_error(events); });
  _buffer_event.enable(EV_READ);
}

void FIX::start() {
  VLOG(1) << "connect("
    "host=\"" << _uri.host << "\", "
    "port=" << _uri.get_port_with_default() <<
    ")";
  _buffer_event.connect(
      _dns_base,
      AF_INET,
      _uri.host,
      _uri.get_port_with_default());
}

void FIX::send(const std::string_view& message) {
  VLOG(4) << "send(length=" << message.length() << ")";
  _buffer_event.write(message.data(), message.length());
  _buffer_event.flush(EV_WRITE, BEV_FLUSH);
}

// bufferevent:

void FIX::on_read() {
  _buffer_event.read(_buffer);
  process_data();
}

void FIX::on_error(int events) {
  if (events & BEV_EVENT_CONNECTED) {
    LOG(INFO) << "CONNECTED";

    auto now = core::get_realtime_clock();

    auto raw_data = Random::create_raw_data(now);
    auto password = Random::create_password(raw_data, _access_secret);

    char buffer[4096];
    auto message = core::fix::Writer(
        buffer,
        std::size(buffer),
        core::fix::MsgType::LOGON,
        SENDER_COMP_ID,
        TARGET_COMP_ID,
        _msg_seq_num)
      .write(core::fix::Field::HEART_BT_INT, uint16_t{10})
      .write(core::fix::Field::RAW_DATA, raw_data)
      .write(core::fix::Field::USERNAME, _access_key)
      .write(core::fix::Field::PASSWORD, password)
      .write(DERIBIT_FIX_FIELD_CANCEL_ON_DISCONNECT, true)
      .finish();

    core::print_memory(message);

    _buffer_event.write(message);
  } else {
    _controller.on_ws_disconnect();
  }
}

void FIX::on_timer() {
  /*
  auto now = core::get_time();
  if (now < _next_update)
    return;
  _next_update = now + PING_FREQUENCY;
  switch (_state) {
    case State::UPGRADED:
      send_ping();
      break;
    default:
      break;
  }
  */
}

// fix:

void FIX::process_data() {
  auto length = _buffer.length();
  if (length == 0)
    return;
  auto buffer = _buffer.pullup(length);
  auto bytes = core::fix::Reader::dispatch(
      overloaded {
        [](
            const core::fix::header_t& header,
            const core::fix::body_t& body) {
          LOG(INFO) << fmt::format("msg_type={} msg_seq_num={}",
              header.msg_type, header.msg_seq_num);
          if (header.msg_type == core::fix::MsgType::LOGON) {
            // for (auto [key, value] : body) { }
          }
        },
        [](const core::fix::value_t& value) {
          LOG(INFO) << "field=" << value.field << ", value=" << value.value;
        },
      },
      buffer,
      length);
  if (bytes > 0) {
    core::print_memory(buffer, bytes);
    core::print_memory_as_cpp_array(buffer, bytes);
    _buffer.drain(bytes);
  }
}

}  // namespace deribit
}  // namespace roq
