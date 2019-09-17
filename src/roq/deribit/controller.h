/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <string>
#include <vector>

#include "roq/core/utils/buffer.h"

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

class Gateway;

class Controller final {
 public:
  Controller(
      Gateway& gateway,
      const std::string_view& access_key,
      const std::string_view& access_secret);

  Controller(Controller&) = delete;
  void operator=(Controller&) = delete;

  void on_timer();

  // rest api:
  void on_rest_connected();
  void on_rest_disconnected();

  // websocket api:
  void on_ws_ready();
  void on_ws_disconnect();

  // fix api:
  void on_fix_connected();
  void on_fix_disconnected();
  void operator()(const fix::ExecutionReport& execution_report, uint64_t seq_num);
  void operator()(const fix::Heartbeat& heartbeat, uint64_t seq_num);
  void operator()(const fix::Logon& logon, uint64_t seq_num);
  void operator()(const fix::Logout& logout, uint64_t seq_num);
  void operator()(const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh, uint64_t seq_num);
  void operator()(const fix::MarketDataRequestReject& market_data_request_reject, uint64_t seq_num);
  void operator()(const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh, uint64_t seq_num);
  void operator()(const fix::OrderCancelReject& order_cancel_reject, uint64_t seq_num);
  void operator()(const fix::PositionReport& position_report, uint64_t seq_num);
  void operator()(const fix::Reject& reject, uint64_t seq_num);
  void operator()(const fix::ResendRequest& resend_request, uint64_t seq_num);
  void operator()(const fix::SecurityList& security_list, uint64_t seq_num);
  void operator()(const fix::TestRequest& test_request, uint64_t seq_num);
  void operator()(const fix::UserResponse& user_response, uint64_t seq_num);

 private:
  Gateway& _gateway;
  std::vector<std::byte> _decode_buffer;
  core::utils::Buffer _encode_buffer;
  std::chrono::nanoseconds _next_update = {};
  // fix:
  const std::string _access_key;
  const std::string _access_secret;
  uint64_t _msg_seq_num = 0;
};

}  // namespace deribit
}  // namespace roq
