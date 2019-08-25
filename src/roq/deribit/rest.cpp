/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/rest.h"

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <utility>

#include "roq/patterns.h"

#include "roq/core/clock.h"

#include "roq/core/http/response.h"

namespace roq {
namespace deribit {

namespace {
constexpr auto TIMER_FREQUENCY = std::chrono::milliseconds{100};
constexpr auto TIMEOUT_SECONDS = std::chrono::seconds{60};
constexpr auto MAX_REQUESTS_PER_SECOND = 3;
}  // namespace

Rest::Rest(
    core::ssl::Context& ssl_context,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    const core::URI& uri)
    : _ssl_context(ssl_context),
      _base(base),
      _dns_base(dns_base),
      _uri(uri),
      _timer(base, EV_PERSIST, [this]() { on_timer(); }) {
  LOG_IF(FATAL, _uri.scheme.compare("https") != 0) <<
    "Expected URI scheme to be \"https\" (got \"" << _uri.scheme << "\")";
  _timer.add(TIMER_FREQUENCY);
}

void Rest::enqueue(
    std::string&& uri,
    success_t&& success,
    failure_t&& failure) {
  switch (_state) {
    case State::DISCONNECTED:
      connect();
      [[ fallthrough ]];
    case State::DISCONNECTING:  // will fail in order
    case State::CONNECTING:
      make_pending(
          std::move(uri),
          std::move(success),
          std::move(failure));
      break;
    case State::CONNECTED:
      if (_waiting.empty() && request(core::http::Method::GET, uri)) {
        make_sent(
            std::move(success),
            std::move(failure));
      } else {
        make_pending(
            std::move(uri),
            std::move(success),
            std::move(failure));
      }
      break;
    default:
      LOG(FATAL) << "Unexpected";
  }
}

void Rest::on_timer() {
  switch (_state) {
    case State::DISCONNECTED:
    case State::CONNECTING:
      break;
    case State::CONNECTED:
      if (_waiting.empty())
        check_timeout();
      else
        process_pending();
      break;
    case State::DISCONNECTING:
      assert(_connection);
      LOG(INFO) << "Disconnected";
      _connection.reset();
      _state = State::DISCONNECTED;
      break;
  }
}

void Rest::check_timeout() {
  assert(_state != State::DISCONNECTING);
  auto now = core::get_time();
  if ((now - _window) < TIMEOUT_SECONDS)
    return;
  LOG(INFO) << "Disconnecting...";
  _state = State::DISCONNECTING;
}

void Rest::connect() {
  assert(!_connection);
  LOG(INFO) << "Connecting...";
  _connection = std::make_unique<HTTPConnection>(
      *this,
      _ssl_context,
      _base);
  _connection->connect(_dns_base, _uri);
  _state = State::CONNECTING;
}

void Rest::process_pending() {
  while (!_waiting.empty()) {
    auto& front = _waiting.front();
    if (!request(core::http::Method::GET, std::get<0>(front)))
      return;
    _sent.push_back(
        std::make_tuple(
            std::move(std::get<1>(front)),
            std::move(std::get<2>(front))));
    _waiting.pop_front();
  }
}

bool Rest::request(
    const core::http::Method& method,
    const std::string_view& path) {
  assert(_state == State::CONNECTED);
  assert(_connection);
  if (throttle()) {
    LOG(WARNING) << "Request is pending due to throttling";
    return false;
  }
  fmt::memory_buffer buffer;
  fmt::format_to(
      buffer,
      "{} {} HTTP/1.1\r\n"
      "Host: {}\r\n"
      "User-Agent: roq-trading/{}\r\n"
      "Accept: */*\r\n"
      "\r\n",
      method,
      path,
      _uri.host,
      ROQ_VERSION);
  VLOG(1) << buffer.size() << " " << buffer.data();
  _connection->write(buffer.data(), buffer.size());
  return true;
}

bool Rest::throttle() {
  auto now = core::get_time();
  auto window = std::chrono::floor<std::chrono::seconds>(now);
  if (_window != window) {
    _window = window;
    _request_count = 0;
  }
  if (MAX_REQUESTS_PER_SECOND <= _request_count)
    return true;
  ++_request_count;
  return false;
}

void Rest::make_pending(
    std::string&& uri,
    success_t&& success,
    failure_t&& failure) {
  _waiting.push_back(
      std::make_tuple(
          std::move(uri),
          std::move(success),
          std::move(failure)));
}

void Rest::make_sent(
    success_t&& success,
    failure_t&& failure) {
  _sent.push_back(
      std::make_tuple(
          std::move(success),
          std::move(failure)));
}

bool Rest::remove_one(
    const core::http::Status& status,
    const std::string_view& body) {
  if (_sent.empty())
    return false;
  auto& front = _sent.front();
  try {
    if (status == core::http::Status::OK) {
      std::get<0>(front)(body);  // success handler
    } else {
      std::get<1>(front)();  // failure handler
    }
  } catch (std::exception& e) {
    LOG(WARNING) << fmt::format("exception, what=\"{}\"", e.what());
  }
  _sent.pop_front();
  return true;
}

void Rest::remove_all() {
  core::http::Status status = core::http::Status::UNKNOWN;
  std::string_view body;
  while (remove_one(status, body)) {
  }
}

void Rest::operator()(const HTTPConnection::connected_t&) {
  assert(_state != State::CONNECTED);
  assert(_sent.empty());
  LOG(INFO) << "Connected";
  _state = State::CONNECTED;
  process_pending();
}

void Rest::operator()(const HTTPConnection::disconnected_t&) {
  assert(_state != State::DISCONNECTING);
  LOG(INFO) << "Disconnecting...";
  _state = State::DISCONNECTING;
}

void Rest::operator()(const HTTPConnection::status_t& status) {
  _status = status.status;
}

void Rest::operator()(const HTTPConnection::header_t&) {
}

void Rest::operator()(const HTTPConnection::body_t& body) {
  std::string_view body_(
      reinterpret_cast<const char *>(body.data),
      body.length);
  if (!remove_one(_status, body_))
    throw std::runtime_error("Unexpected [process]");
  _status = core::http::Status::UNKNOWN;
}

}  // namespace deribit
}  // namespace roq
