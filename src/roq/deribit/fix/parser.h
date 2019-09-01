/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/logging.h"

#include "roq/deribit/fix/heartbeat.h"
#include "roq/deribit/fix/logon.h"
#include "roq/deribit/fix/logout.h"

namespace roq {
namespace deribit {
namespace fix {

struct Parser final {
  template <typename H>
  static void dispatch(
      H&& handler,
      const core::fix::header_t& header,
      const core::fix::body_t& body) {
    switch (header.msg_type) {
      case core::fix::MsgType::LOGON: {
        Logon result;
        Logon::parse(result, header, body);
        handler(result);
        break;
      }
      case core::fix::MsgType::LOGOUT: {
        Logout result;
        Logout::parse(result, header, body);
        handler(result);
        break;
      }
      case core::fix::MsgType::HEARTBEAT: {
        Heartbeat result;
        Heartbeat::parse(result, header, body);
        handler(result);
        break;
      }
      default: {
        LOG(WARNING) << fmt::format(
            "Unknown msg_type={}",
            header.msg_type);
      }
    }
  }
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq
