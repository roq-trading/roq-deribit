/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <cstddef>
#include <vector>

#include "roq/logging.h"

#include "roq/deribit/fix/execution_report.h"
#include "roq/deribit/fix/heartbeat.h"
#include "roq/deribit/fix/logon.h"
#include "roq/deribit/fix/logout.h"
#include "roq/deribit/fix/market_data_incremental_refresh.h"
#include "roq/deribit/fix/market_data_request_reject.h"
#include "roq/deribit/fix/market_data_snapshot_full_refresh.h"
#include "roq/deribit/fix/order_cancel_reject.h"
#include "roq/deribit/fix/position_report.h"
#include "roq/deribit/fix/reject.h"
#include "roq/deribit/fix/resend_request.h"
#include "roq/deribit/fix/security_list.h"
#include "roq/deribit/fix/test_request.h"
#include "roq/deribit/fix/user_response.h"

namespace roq {
namespace deribit {
namespace fix {

struct Parser final {
  template <typename H>
  static void dispatch(
      H&& handler,
      const core::fix::message_t& message,
      std::vector<std::byte>& buffer) {
    switch (message.header.msg_type) {
      case core::fix::MsgType::EXECUTION_REPORT: {
        handler(ExecutionReport::parse(message));
        break;
      }
      case core::fix::MsgType::HEARTBEAT: {
        handler(Heartbeat::parse(message));
        break;
      }
      case core::fix::MsgType::LOGON: {
        handler(Logon::parse(message));
        break;
      }
      case core::fix::MsgType::LOGOUT: {
        handler(Logout::parse(message));
        break;
      }
      case core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH: {
        handler(MarketDataIncrementalRefresh::parse(message, buffer));
        break;
      }
      case core::fix::MsgType::MARKET_DATA_REQUEST_REJECT: {
        handler(MarketDataRequestReject::parse(message));
        break;
      }
      case core::fix::MsgType::MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
        handler(MarketDataSnapshotFullRefresh::parse(message, buffer));
        break;
      }
      case core::fix::MsgType::ORDER_CANCEL_REJECT: {
        handler(OrderCancelReject::parse(message));
        break;
      }
      case core::fix::MsgType::POSITION_REPORT: {
        handler(PositionReport::parse(message, buffer));
        break;
      }
      case core::fix::MsgType::REJECT: {
        handler(Reject::parse(message));
        break;
      }
      case core::fix::MsgType::RESEND_REQUEST: {
        handler(ResendRequest::parse(message));
        break;
      }
      case core::fix::MsgType::SECURITY_LIST: {
        handler(SecurityList::parse(message, buffer));
        break;
      }
      case core::fix::MsgType::TEST_REQUEST: {
        handler(TestRequest::parse(message));
        break;
      }
      case core::fix::MsgType::USER_RESPONSE: {
        handler(UserResponse::parse(message));
        break;
      }
      default: {
        LOG(WARNING) << fmt::format(
            "Unknown msg_type={}",
            message.header.msg_type);
      }
    }
  }
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq
