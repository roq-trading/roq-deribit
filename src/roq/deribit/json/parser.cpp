/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/json/parser.h"

#include "roq/compat.h"

#include "roq/deribit/json/channel.h"
#include "roq/deribit/json/field.h"
#include "roq/deribit/json/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace json {

namespace {
template <typename T>
void dispatch_ticker(
    Parser::Handler& handler,
    T& value,
    const server::TraceInfo& trace_info) {
  Ticker ticker(value);
  handler(
      ticker,
      trace_info);
}
}  // namespace

void Parser::dispatch(
    Parser::Handler& handler,
    core::json::value_t& value,
    [[ maybe_unused ]] core::json::Buffer& buffer,
    const server::TraceInfo& trace_info) {
  // note! message is nested / channel name is at level 2
  auto message = core::json::get<std::string_view>(value);
  auto channel = Channel::UNDEFINED;
  bool dispatched = false;
  for (int i = 0; i < 2 && dispatched == false; ++i) {
    core::json::Parser parser(message);
    auto root = parser.root();
    for (auto [key, value] : std::get<core::json::object_t>(root)) {
      auto field = Field(key);
      switch (field) {
        case Field::UNDEFINED:
          LOG(FATAL)("Unexpected");
          break;
        case Field::UNKNOWN:
          DLOG(FATAL)(
              R"(Unknown key="{}")",
              key);
          break;
        case Field::CHANNEL: {
          auto name = std::get<std::string_view>(value);
          auto delim = name.find_first_of('.');
          if (delim != name.npos) {
            channel = Channel(name.substr(0, delim));
          } else {
            channel = Channel::UNKNOWN;
          }
          LOG_IF(WARNING, channel == Channel::UNKNOWN)(
              R"(Can't parse channel="{}")",
              name);
          break;
        }
        case Field::DATA:
          if (channel != Channel::UNDEFINED) {
            switch (channel) {
              case Channel::UNDEFINED:
                break;  // not ready
              case Channel::UNKNOWN:
                DLOG(FATAL)("Unknown channel");
                break;
              case Channel::TICKER:
                dispatched = true;
                dispatch_ticker(
                    handler,
                    value,
                    trace_info);
                break;
            }
          }
          break;
      }
    }
  }
  if (dispatched)
    return;
  LOG(WARNING)(
      R"(message="{}")",
      message);
  LOG(FATAL)("Unexpected");
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
