/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "roq/core/uri.h"
#include "roq/core/ssl/ssl.h"
#include "roq/core/event/event.h"
#include "roq/core/http/response.h"
#include "roq/core/ws/decoder.h"

#include "roq/core/fix/common.h"

#include "roq/deribit/controller.h"

namespace roq {
namespace deribit {

class FIX final {
 public:
  FIX(
      Controller& controller,
      core::ssl::Context& ssl_context,
      core::event::Base& base,
      core::event::DNSBase& dns_base,
      const core::URI& uri,
      const std::string_view& access_key,
      const std::string_view& access_secret);

  void start();
  void send(const std::string_view& message);

 private:
  void on_read();
  void on_error(int err);

  void on_timer();

  void process_data();

  void send_reject(
      uint64_t ref_seq_num,
      const std::string_view& ref_msg_type,
      const std::string_view& text);
  void send_reject(
      uint64_t ref_seq_num,
      const core::fix::MsgType& msg_type,
      const std::string_view& text);
  void send_logon(uint16_t heart_bt_int, bool cancel_on_disconnect);
  void send_logout(const std::string_view& text);
  void send_heartbeat(const std::string_view& test_req_id);
  void send_test_request(const std::string_view& test_req_id);
  void send_security_list_request(
      const std::string_view& security_req_id);
  void send_market_data_request(
      const std::string_view& md_req_id,
      const std::string_view& symbol);
  void send_user_request(const std::string_view& user_request_id);
  void send_request_for_positions(
      const std::string_view& pos_req_id,
      const core::fix::PosReqType& pos_req_type);
  void send_order_mass_status_request(
      const std::string_view& mass_status_req_id,
      const core::fix::MassStatusReqType& mass_status_req_type);
  void send_new_order_single(
      const std::string_view& cl_ord_id,
      const core::fix::Side& side,
      double order_qty,
      double price,
      const std::string_view& symbol,
      const core::fix::OrdType& ord_type,
      const core::fix::TimeInForce& time_in_force,
      const std::string_view& deribit_label);
  void send_order_cancel_replace_request(
      const std::string_view& cl_ord_id,
      const std::string_view& orig_cl_ord_id,
      const core::fix::Side& side,
      double order_qty,
      const core::fix::OrdType& ord_type,
      double price,
      const std::string_view& symbol,
      std::chrono::nanoseconds transact_time = {});
  void send_order_cancel_request(
      const std::string_view& cl_ord_id,
      const std::string_view& orig_cl_ord_id);

 private:
  Controller& _controller;
  core::ssl::Connection _ssl_connection;
  core::event::DNSBase& _dns_base;
  const core::URI _uri;
  const std::string _access_key;
  const std::string _access_secret;
  core::event::Timer _timer;
  core::event::BufferEvent _buffer_event;
  core::event::Buffer _buffer;
  std::vector<std::byte> _decode_buffer;
  uint64_t _msg_seq_num = 0;
  std::chrono::nanoseconds _next_update = {};
};

}  // namespace deribit
}  // namespace roq
