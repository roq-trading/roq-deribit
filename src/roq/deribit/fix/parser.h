/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/logging.h"

#include "roq/deribit/fix/execution_report.h"
#include "roq/deribit/fix/heartbeat.h"
#include "roq/deribit/fix/logon.h"
#include "roq/deribit/fix/logout.h"
#include "roq/deribit/fix/market_data_incremental_refresh.h"
#include "roq/deribit/fix/market_data_request_reject.h"
#include "roq/deribit/fix/market_data_snapshot_full_refresh.h"
#include "roq/deribit/fix/position_report.h"
#include "roq/deribit/fix/resend_request.h"
#include "roq/deribit/fix/security_list.h"
#include "roq/deribit/fix/test_request.h"

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
      case core::fix::MsgType::EXECUTION_REPORT: {
        ExecutionReport result;
        ExecutionReport::parse(result, header, body);
        handler(result);
        break;
      }
      case core::fix::MsgType::HEARTBEAT: {
        Heartbeat result;
        Heartbeat::parse(result, header, body);
        handler(result);
        break;
      }
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
      case core::fix::MsgType::MARKET_DATA_INCREMENTAL_REFRESH: {
        MarketDataIncrementalRefresh result;
        MarketDataIncrementalRefresh::parse(result, header, body);
        handler(result);
        break;
      }
      case core::fix::MsgType::MARKET_DATA_REQUEST_REJECT: {
        MarketDataRequestReject result;
        MarketDataRequestReject::parse(result, header, body);
        handler(result);
        break;
      }
      case core::fix::MsgType::MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
        MarketDataSnapshotFullRefresh result;
        MarketDataSnapshotFullRefresh::parse(result, header, body);
        handler(result);
        break;
      }
      case core::fix::MsgType::POSITION_REPORT: {
        PositionReport result;
        PositionReport::parse(result, header, body);
        handler(result);
        break;
      }
      case core::fix::MsgType::RESEND_REQUEST: {
        ResendRequest result;
        ResendRequest::parse(result, header, body);
        handler(result);
        break;
      }
      case core::fix::MsgType::SECURITY_LIST: {
        SecurityList result;
        SecurityList::parse(result, header, body);
        handler(result);
        break;
      }
      case core::fix::MsgType::TEST_REQUEST: {
        TestRequest result;
        TestRequest::parse(result, header, body);
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
